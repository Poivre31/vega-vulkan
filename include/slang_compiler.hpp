#pragma once

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
#include <cstdint>
#include <vector>
#include "console/console.hpp"
#include "layer.hpp"
#include "vulkan/vulkan.hpp"
#include <io.hpp>

// const char* const simple_shader =
//     "RWStructuredBuffer<float> result;"
//     "[shader(\"compute\")]"
//     "[numthreads(1,1,1)]"
//     "void computeMain(uint3 threadId : SV_DispatchThreadID)"
//     "{"
//     "    result[threadId.x] = threadId.x;"
//     "}";

class slang_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  bool init() noexcept final { return load_shaders(); }
  void update(double dt) noexcept final {
    const bool* key_states = SDL_GetKeyboardState(nullptr);

    if (get_app_context()->recompile_shaders) {
      _console->info("Recompiling shaders...");
      if (!load_shaders()) {
        _console->error("Shader recompilation failed, keeping old pipeline");
        get_app_context()->vulkan.recreate_graphics_pipeline = false;
      } else {
        get_app_context()->vulkan.recreate_graphics_pipeline = true;
      }
      get_app_context()->recompile_shaders = false;
    }
  }
  void cleanup() noexcept final {}

 private:
  static void diagnose_if_needed(slang::IBlob* diagnostics_blob) {
    if (diagnostics_blob != nullptr) {
      _console->error(
          "Slang diagnositcs error: {}",
          reinterpret_cast<const char*>(diagnostics_blob->getBufferPointer())
      );
    }
  }

  bool load_shaders() {
    // CREATE SESSION
    Slang::ComPtr<slang::IGlobalSession> global_session;
    slang::createGlobalSession(global_session.writeRef());

    slang::TargetDesc target_description{
        .format = SLANG_SPIRV, .profile = global_session->findProfile("unknown")
    };
#ifdef NDEBUG
    std::array<slang::CompilerOptionEntry, 2> options{
        slang::CompilerOptionEntry{
            .name = slang::CompilerOptionName::Optimization,
            .value =
                slang::CompilerOptionValue {
                  .kind      = slang::CompilerOptionValueKind::Int,
                  .intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE
                }
        },
        slang::CompilerOptionEntry {
          .name = slang::CompilerOptionName::DebugInformation,
          .value =
              slang::CompilerOptionValue{
                  .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 0
              }
        }
    };
#else
    std::array<slang::CompilerOptionEntry, 2> options{
        slang::CompilerOptionEntry{
            .name = slang::CompilerOptionName::Optimization,
            .value =
                slang::CompilerOptionValue{
                    .kind      = slang::CompilerOptionValueKind::Int,
                    .intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE
                }
        },
        slang::CompilerOptionEntry{
            .name  = slang::CompilerOptionName::DebugInformation,
            .value = slang::CompilerOptionValue{
                .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1
            }
        }
    };
#endif

    slang::SessionDesc session_description{
        .targets                  = &target_description,
        .targetCount              = 1,
        .defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
        .preprocessorMacros       = nullptr,
        .preprocessorMacroCount   = 0,
        .compilerOptionEntries    = options.data(),
        .compilerOptionEntryCount = options.size(),
    };

    Slang::ComPtr<slang::ISession> session;
    global_session->createSession(session_description, session.writeRef());

    // LOAD MODULE

    auto simple_shader = read_file("resources/shaders/lit_shader.slang");
    Slang::ComPtr<slang::IModule> slang_module;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      const char* module_name = "simple";
      const char* module_path = "simple.slang";
      slang_module            = session->loadModuleFromSourceString(
          module_name, module_path, simple_shader.data(), diagnostics.writeRef()
      );
      diagnose_if_needed(diagnostics);
      if (!slang_module) {
        _console->error("Module [{}] loading failed ('{}')", module_name, module_path);
        return false;
      } else {
        _console->info("Loaded module [{}]", module_name);
      }
    }

    // FIND ENTRY POINT
    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      std::vector<const char*> entry_point_names{"vertMain", "fragMain"};
      for (const char* entry_point_name : entry_point_names) {
        entry_points.emplace_back(nullptr);
        slang_module->findEntryPointByName(entry_point_name, entry_points.back().writeRef());
        diagnose_if_needed(diagnostics);
        if (!entry_points.back()) {
          _console->error(
              "Module [{}] failed to find entry point [{}]",
              slang_module->getName(),
              entry_point_name
          );
          return false;
        }
      }
    }

    std::vector<slang::IComponentType*> component_types{slang_module};
    component_types.insert(component_types.end(), entry_points.begin(), entry_points.end());

    Slang::ComPtr<slang::IComponentType> composed_program;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      auto result = session->createCompositeComponentType(
          component_types.data(),
          SlangInt(component_types.size()),
          composed_program.writeRef(),
          diagnostics.writeRef()
      );
      diagnose_if_needed(diagnostics);
      if (SLANG_FAILED(result)) {
        _console->error("Shader program composition failed");
        return false;
      }
    }

    Slang::ComPtr<slang::IComponentType> linked_program;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      auto result = composed_program->link(linked_program.writeRef(), diagnostics.writeRef());
      diagnose_if_needed(diagnostics);
      if (SLANG_FAILED(result)) {
        _console->error("Shader program link failed");
        return false;
      }
    }

    Slang::ComPtr<slang::IBlob> spirv_code;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      auto result = linked_program->getTargetCode(0, spirv_code.writeRef(), diagnostics.writeRef());
      diagnose_if_needed(diagnostics);
      if (SLANG_FAILED(result)) {
        _console->error("Shader program spirv failed");
        return false;
      }
    }

    get_app_context()->shader_modules.clear();
    get_app_context()->shader_modules.emplace_back(
        reinterpret_cast<const uint32_t*>(spirv_code->getBufferPointer()),
        reinterpret_cast<const uint32_t*>(spirv_code->getBufferPointer())
            + spirv_code->getBufferSize()
    );

    return true;
  }

  static inline vega_console _console = console::create("Slang");
};
