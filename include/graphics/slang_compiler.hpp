#pragma once

#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
#include <sys/types.h>
#include <cstdint>
#include <exception>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>
#include <io.hpp>
#include <console/console.hpp>

#include "application/layer.hpp"

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
  slang_layer(application_context* context) : Ilayer(context) {
    // CREATE SESSION
    slang::createGlobalSession(_global_session.writeRef());
  }
  bool init() noexcept final { return load_shaders(); }
  void update(double dt) noexcept final {
    const bool* key_states = SDL_GetKeyboardState(nullptr);

    if (get_app_context()->vulkan.recompile_shaders) {
      _console->info("Recompiling shaders...");
      if (!load_shaders()) {
        _console->error("Shader recompilation failed, keeping old pipeline");
        get_app_context()->vulkan.recreate_graphics_pipeline = false;
      } else {
        get_app_context()->vulkan.recreate_graphics_pipeline = true;
      }
      get_app_context()->vulkan.recompile_shaders = false;
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

  static std::vector<char> compile_program(
      const Slang::ComPtr<slang::ISession>& _session,
      const char* shader_directory,
      const char* shader_name,
      const char* module_name,
      const std::vector<const char*>& entry_point_names
  ) {
    // LOAD MODULE
    auto shader_source = read_file(std::string(shader_directory) + shader_name);
    Slang::ComPtr<slang::IModule> shader_module;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      const char* module_path = shader_name;
      shader_module           = _session->loadModuleFromSourceString(
          module_name, module_path, shader_source.c_str(), diagnostics.writeRef()
      );
      diagnose_if_needed(diagnostics);
      if (!shader_module) {
        _console->error("Module [{}] loading failed ('{}')", module_name, module_path);
        throw std::runtime_error("");
      } else {
        _console->info("Loaded module [{}]", module_name);
      }
    }

    // FIND ENTRY POINT
    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      for (const char* entry_point_name : entry_point_names) {
        entry_points.emplace_back(nullptr);
        shader_module->findEntryPointByName(entry_point_name, entry_points.back().writeRef());
        diagnose_if_needed(diagnostics);
        if (!entry_points.back()) {
          _console->error(
              "Module [{}] failed to find entry point [{}]",
              shader_module->getName(),
              entry_point_name
          );
          throw std::runtime_error("");
        }
      }
    }

    std::vector<slang::IComponentType*> component_types{shader_module};
    component_types.insert(component_types.end(), entry_points.begin(), entry_points.end());

    Slang::ComPtr<slang::IComponentType> composed_program;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      auto result = _session->createCompositeComponentType(
          component_types.data(),
          SlangInt(component_types.size()),
          composed_program.writeRef(),
          diagnostics.writeRef()
      );
      diagnose_if_needed(diagnostics);
      if (SLANG_FAILED(result)) {
        _console->error("Shader program composition failed");
        throw std::runtime_error("");
      }
    }

    Slang::ComPtr<slang::IComponentType> linked_program;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      auto result = composed_program->link(linked_program.writeRef(), diagnostics.writeRef());
      diagnose_if_needed(diagnostics);
      if (SLANG_FAILED(result)) {
        _console->error("Shader program link failed");
        throw std::runtime_error("");
      }
    }

    Slang::ComPtr<slang::IBlob> spirv_code;
    {
      Slang::ComPtr<slang::IBlob> diagnostics;
      auto result = linked_program->getTargetCode(0, spirv_code.writeRef(), diagnostics.writeRef());
      diagnose_if_needed(diagnostics);
      if (SLANG_FAILED(result)) {
        _console->error("Shader program spirv failed");
        throw std::runtime_error("");
      }
    }

    std::ofstream spirv_debug(
        std::string(shader_directory) + module_name + std::string(".spv"), std::ios::binary
    );
    spirv_debug.write(
        reinterpret_cast<const char*>(spirv_code->getBufferPointer()),
        static_cast<std::streamsize>(spirv_code->getBufferSize())
    );
    spirv_debug.close();

    return {
        reinterpret_cast<const char*>(spirv_code->getBufferPointer()),
        reinterpret_cast<const char*>(spirv_code->getBufferPointer()) + spirv_code->getBufferSize()
    };
  }

  bool load_shaders() {
    auto timer = scoped_timer("shader-compiling");

    slang::TargetDesc target_description{
        .format = SLANG_SPIRV, .profile = _global_session->findProfile("unknown")
    };
#ifdef NDEBUG
    std::vector<slang::CompilerOptionEntry> options{
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
    std::vector<slang::CompilerOptionEntry> options{
        slang::CompilerOptionEntry{
            .name = slang::CompilerOptionName::ForceCLayout,
            .value =
                slang::CompilerOptionValue{
                    .kind      = slang::CompilerOptionValueKind::Int,
                    .intValue0 = 1,
                }
        },
        slang::CompilerOptionEntry{
            .name = slang::CompilerOptionName::Optimization,
            .value =
                slang::CompilerOptionValue{
                    .kind      = slang::CompilerOptionValueKind::Int,
                    .intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE,
                }
        },
        slang::CompilerOptionEntry{
            .name  = slang::CompilerOptionName::DebugInformation,
            .value = slang::CompilerOptionValue{
                .kind      = slang::CompilerOptionValueKind::Int,
                .intValue0 = 1,
            }
        },
    };
#endif

    slang::SessionDesc session_description{
        .targets                  = &target_description,
        .targetCount              = 1,
        .defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
        .preprocessorMacros       = nullptr,
        .preprocessorMacroCount   = 0,
        .compilerOptionEntries    = options.data(),
        .compilerOptionEntryCount = static_cast<uint32_t>(options.size()),
    };

    Slang::ComPtr<slang::ISession> _session;
    _global_session->createSession(session_description, _session.writeRef());

    // LOAD MODULE
    try {
      get_app_context()->shader_modules.clear();
      get_app_context()->shader_modules.emplace_back(
          std::move(compile_program(
              _session, "resources/shaders/", "lit_shader.slang", "lit", {"fragMain", "vertMain"}
          ))
      );
      get_app_context()->shader_modules.emplace_back(
          std::move(compile_program(
              _session, "resources/shaders/", "pp_shader.slang", "pp", {"computeMain"}
          ))
      );
    } catch (std::exception e) {
      _console->error("Failed to load, compile and link shaders, exiting program");
      return false;
    }

    _console->info("Successfully loaded, compiled and linked shaders");
    return true;
  }

  static inline vega_console _console = console::create("Slang");
  Slang::ComPtr<slang::IGlobalSession> _global_session;
};
