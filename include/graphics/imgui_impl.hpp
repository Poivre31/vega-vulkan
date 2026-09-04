#pragma once

#include <SDL3/SDL_video.h>
#include <cstddef>
#include <stdexcept>
#include "application/context.hpp"

#include "graphics/config.hpp"
#include "graphics/context.hpp"
#define IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

static void check_vk_result(VkResult err) {
  if (err == VK_SUCCESS) {
    return;
  } else {
    throw std::runtime_error(fmt::format("Vulkan error : {}", vk::to_string(vk::Result(err))));
  }
}

void imgui_init(SDL_Window* window, vulkan_context& vk_context) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io     = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui_ImplSDL3_InitForVulkan(window);
  auto format = VkFormat(vulkan_config::color_format);
  auto depth  = VkFormat(vulkan_config::depth_format);
  ImGui_ImplVulkan_InitInfo init_info{
      .Instance       = **vk_context.instance,
      .PhysicalDevice = **vk_context.physical_device,
      .Device         = **vk_context.device,
      .QueueFamily    = vk_context.graphics_queue_family,
      .Queue          = **vk_context.graphics_queue,
      .DescriptorPool = **vk_context.imgui_descriptor_pool,
      .MinImageCount  = 2,
      .ImageCount     = vk_context.image_count,
      .PipelineCache  = VK_NULL_HANDLE,
      .PipelineInfoMain =
          ImGui_ImplVulkan_PipelineInfo{
              .RenderPass  = NULL,
              .MSAASamples = static_cast<VkSampleCountFlagBits>(
                  static_cast<VkFlags>(vk_context.msaa_sample_count)
              ),
              .PipelineRenderingCreateInfo =
                  {.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                   .colorAttachmentCount    = 1,
                   .pColorAttachmentFormats = &format,
                   .depthAttachmentFormat   = depth},
          },
      .UseDynamicRendering = vk::True,
      .Allocator           = *vk_context.allocator->getAllocationCallbacks(),
      .CheckVkResultFn     = check_vk_result,
  };
  ImGui_ImplVulkan_Init(&init_info);
}

void imgui_begin_frame() {
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

void imgui_end_frame(vk::raii::CommandBuffer& cmd) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
}

void imgui_cleanup() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}