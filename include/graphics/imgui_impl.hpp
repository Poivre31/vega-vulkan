#pragma once

#include <cstddef>
#include "application/context.hpp"

#include "console/console.hpp"
#include "graphics/config.hpp"
#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

class imgui_wrapper {
 public:
  static void init(application_context* context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io     = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplSDL3_InitForVulkan(context->window);
    auto format = VkFormat(vulkan_config::color_format);
    auto depth  = VkFormat(vulkan_config::depth_format);
    ImGui_ImplVulkan_InitInfo init_info{
        .Instance       = **context->vulkan.instance,
        .PhysicalDevice = **context->vulkan.physical_device,
        .Device         = **context->vulkan.device,
        .QueueFamily    = context->vulkan.graphics_queue_family,
        .Queue          = **context->vulkan.graphics_queue,
        .DescriptorPool = **context->vulkan.imgui_descriptor_pool,
        .MinImageCount  = 2,
        .ImageCount     = context->vulkan.image_count,
        .PipelineCache  = VK_NULL_HANDLE,
        .PipelineInfoMain =
            ImGui_ImplVulkan_PipelineInfo{
                .RenderPass  = NULL,
                .MSAASamples = static_cast<VkSampleCountFlagBits>(
                    static_cast<VkFlags>(context->vulkan.msaa_sample_count)
                ),
                .PipelineRenderingCreateInfo =
                    {.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                     .colorAttachmentCount    = 1,
                     .pColorAttachmentFormats = &format,
                     .depthAttachmentFormat   = depth},
            },
        .UseDynamicRendering = vk::True,
        .Allocator           = *context->vulkan.allocator->getAllocationCallbacks(),
        .CheckVkResultFn     = check_vk_result,
    };
    ImGui_ImplVulkan_Init(&init_info);
  }

  static void check_vk_result(VkResult err) {
    if (err == VK_SUCCESS) {
      return;
    } else {
      _console->error("Vulkan error : {}", vk::to_string(vk::Result(err)));
    }
    // if (err < 0) {
    //   std::abort();
    // }
  }

  static void begin_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    // ImGui::DockSpaceOverViewport();
    ImGui::ShowDemoWindow();
    auto& io = ImGui::GetIO();
    // if (io.WantCaptureMouse) {
    //   _console->info("idk");
    // }
  }

  static void end_frame(vk::raii::CommandBuffer& cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
  }

  static void cleanup() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }

 private:
  static inline vega_console _console = console::create("ImGui");
};