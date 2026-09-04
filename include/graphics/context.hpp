#pragma once

#include <cstdint>
#include "graphics.hpp"
#include "vulkan/vulkan.hpp"

struct vulkan_context {
  vk::raii::Instance* instance              = nullptr;
  vk::raii::PhysicalDevice* physical_device = nullptr;
  uint32_t graphics_queue_family            = ~0;
  vk::raii::Queue* graphics_queue           = nullptr;
  vk::raii::Device* device                  = nullptr;
  vma::raii::Allocator* allocator           = nullptr;
  vk::raii::CommandPool* command_pool       = nullptr;

  vk::raii::DescriptorPool* imgui_descriptor_pool = nullptr;
  vk::raii::DescriptorSets* descriptor_sets       = nullptr;

  uint32_t image_count{};
  vk::SampleCountFlags msaa_sample_count = vk::SampleCountFlagBits::e1;
  vk::raii::Pipeline* graphics_pipeline  = nullptr;

  bool recreate_swapchain         = false;
  bool recreate_graphics_pipeline = false;
  bool recompile_shaders          = false;
};