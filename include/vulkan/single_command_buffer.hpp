#pragma once
#include "graphics.hpp"
#include "vulkan_context.hpp"

vk::raii::CommandBuffer
begin_single_command_buffer(vk::raii::CommandPool& command_pool, vk::raii::Device& device) {
  vk::CommandBufferAllocateInfo command_buffer_info{
      .commandPool        = command_pool,
      .level              = vk::CommandBufferLevel::ePrimary,
      .commandBufferCount = 1,
  };
  vk::raii::CommandBuffer command_buffer = std::move(
      vk::raii::CommandBuffers(device, command_buffer_info)[0]
  );

  vk::CommandBufferBeginInfo command_buffer_begin{
      .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
  };
  command_buffer.begin(command_buffer_begin);
  return std::move(command_buffer);
}

vk::raii::CommandBuffer begin_single_command_buffer(vulkan_context context) {
  return begin_single_command_buffer(*context.command_pool, *context.device);
}

void submit_single_command_buffer(
    vk::raii::Queue& queue,
    vk::raii::CommandBuffer&& command_buffer  // NOLINT
) {
  command_buffer.end();
  vk::SubmitInfo submit_info{.commandBufferCount = 1, .pCommandBuffers = &*command_buffer};
  queue.submit(submit_info);
  queue.waitIdle();
}

void submit_single_command_buffer(
    vulkan_context context,
    vk::raii::CommandBuffer&& command_buffer  // NOLINT
) {
  submit_single_command_buffer(*context.graphics_queue, std::move(command_buffer));
}