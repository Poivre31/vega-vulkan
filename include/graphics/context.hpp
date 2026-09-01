#pragma once

#include "graphics.hpp"

struct vulkan_context {
  vk::raii::PhysicalDevice* physical_device = nullptr;
  uint32_t graphics_queue_family            = ~0;
  vk::raii::Queue* graphics_queue           = nullptr;
  vk::raii::Device* device                  = nullptr;
  vma::raii::Allocator* allocator           = nullptr;
  vk::raii::CommandPool* command_pool       = nullptr;

  vk::raii::DescriptorSets* descriptor_sets = nullptr;

  bool recreate_graphics_pipeline = false;
};