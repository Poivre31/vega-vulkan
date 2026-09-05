#pragma once

#include <cstdint>

#include "graphics.hpp"
#include "config.hpp"

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

  vk::raii::Pipeline* graphics_pipeline = nullptr;

  dynamic_config config{};

  bool recreate_swapchain         = false;
  bool recreate_graphics_pipeline = false;
  bool update_imgui               = false;
  bool recompile_shaders          = false;
};