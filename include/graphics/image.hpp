#pragma once

#include "resources/stb_image.hpp"

#include "graphics.hpp"
#include "single_command_buffer.hpp"
#include "gpu_objects.hpp"
#include "context.hpp"

void transition_image_layout(
    const vk::Image& image,
    vk::raii::CommandBuffer& command_buffer,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags aspect
) {
  vk::ImageMemoryBarrier2 barrier = {
      .srcStageMask        = src_stage_mask,
      .srcAccessMask       = src_access_mask,
      .dstStageMask        = dst_stage_mask,
      .dstAccessMask       = dst_access_mask,
      .oldLayout           = old_layout,
      .newLayout           = new_layout,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image               = image,
      .subresourceRange    = {
          .aspectMask     = aspect,
          .baseMipLevel   = 0,
          .levelCount     = 1,
          .baseArrayLayer = 0,
          .layerCount     = 1
      }
  };
  vk::DependencyInfo dependency_info = {
      .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier
  };
  command_buffer.pipelineBarrier2(dependency_info);
}

gpu_image create_image(
    vma::raii::Allocator& allocator,
    vk::raii::Device& device,
    vk::Format format,
    uint32_t width,
    uint32_t height,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspect
) {
  vk::ImageCreateInfo image_info{
      .imageType   = vk::ImageType::e2D,
      .format      = format,
      .extent      = {.width = width, .height = height, .depth = 1},
      .mipLevels   = 1,
      .arrayLayers = 1,
      .samples     = vk::SampleCountFlagBits::e1,
      .tiling      = vk::ImageTiling::eOptimal,
      .usage       = usage,
      .sharingMode = vk::SharingMode::eExclusive
  };
  vma::AllocationCreateInfo allocation_info{.usage = vma::MemoryUsage::eAutoPreferDevice};
  auto image = vma::raii::Image(allocator, image_info, allocation_info);

  vk::ImageViewCreateInfo view_info{
      .image            = image,
      .viewType         = vk::ImageViewType::e2D,
      .format           = format,
      .subresourceRange = {
          .aspectMask     = aspect,
          .baseMipLevel   = 0,
          .levelCount     = 1,
          .baseArrayLayer = 0,
          .layerCount     = 1
      }
  };
  auto view = vk::raii::ImageView(device, view_info);

  return {.image = std::move(image), .view = std::move(view)};
}

gpu_image load_texture_to_gpu(const vulkan_context& context, stb_image&& cpu_texture) {
  stb_image texture = std::move(cpu_texture);
  uint64_t size     = texture.byte_size();
  vk::BufferCreateInfo staging_buffer_info{
      .size        = size,
      .usage       = vk::BufferUsageFlagBits::eTransferSrc,
      .sharingMode = vk::SharingMode::eExclusive
  };
  vma::AllocationCreateInfo staging_allocation_info{
      .flags = vma::AllocationCreateFlagBits::eMapped
               | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
      .usage = vma::MemoryUsage::eAutoPreferHost
  };
  auto staging_buffer = context.allocator->createBuffer(
      staging_buffer_info, staging_allocation_info
  );
  staging_buffer.getAllocation().copyFromMemory(texture.data(), 0, size);

  auto image = create_image(
      *context.allocator,
      *context.device,
      vk::Format::eR8G8B8A8Srgb,
      texture.width(),
      texture.height(),
      vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
      vk::ImageAspectFlagBits::eColor
  );

  auto cmd = begin_transient_command_buffer(context);
  transition_image_layout(
      image.image,
      cmd,
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eTransferDstOptimal,
      {},
      vk::AccessFlagBits2::eTransferRead,
      vk::PipelineStageFlagBits2::eTopOfPipe,
      vk::PipelineStageFlagBits2::eTransfer,
      vk::ImageAspectFlagBits::eColor
  );
  cmd.copyBufferToImage(
      staging_buffer,
      image.image,
      vk::ImageLayout::eTransferDstOptimal,
      {vk::BufferImageCopy{
          .imageSubresource =
              {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .layerCount = 1},
          .imageExtent = {
              .width  = static_cast<uint32_t>(texture.width()),
              .height = static_cast<uint32_t>(texture.height()),
              .depth  = 1U
          }
      }}
  );
  transition_image_layout(
      image.image,
      cmd,
      vk::ImageLayout::eTransferDstOptimal,
      vk::ImageLayout::eShaderReadOnlyOptimal,
      vk::AccessFlagBits2::eTransferWrite,
      vk::AccessFlagBits2::eShaderRead,
      vk::PipelineStageFlagBits2::eTransfer,
      vk::PipelineStageFlagBits2::eFragmentShader,
      vk::ImageAspectFlagBits::eColor
  );
  submit_single_command_buffer(context, std::move(cmd));
  return std::move(image);
}
