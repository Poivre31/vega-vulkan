#pragma once

#include "graphics.hpp"
#include "gpu_objects.hpp"

enum class layout_transition : uint8_t { dst_to_src, src_to_shader_read, dst_to_shader_read };

const std::unordered_map<layout_transition, std::pair<vk::ImageLayout, vk::ImageLayout>>
    associated_transition_layouts{
        {layout_transition::dst_to_src,
         {vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal}},
        {layout_transition::src_to_shader_read,
         {vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal}},
        {layout_transition::dst_to_shader_read,
         {vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal}},
    };

void transition_image_layout(
    const vk::Image& image,
    vk::raii::CommandBuffer& command_buffer,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags aspect,
    uint32_t mip_level       = 0,
    uint32_t mip_level_count = 1
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
          .baseMipLevel   = mip_level,
          .levelCount     = mip_level_count,
          .baseArrayLayer = 0,
          .layerCount     = 1
      }
  };
  vk::DependencyInfo dependency_info = {
      .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier
  };
  command_buffer.pipelineBarrier2(dependency_info);
}

void transition_image_layout(
    const vk::Image& image,
    vk::raii::CommandBuffer& command_buffer,
    layout_transition transition,
    vk::ImageAspectFlags aspect,
    uint32_t mip_level       = 0,
    uint32_t mip_level_count = 1
) {
  auto [old_layout, new_layout] = associated_transition_layouts.at(transition);
  vk::AccessFlags2 src_access_mask, dst_access_mask;
  vk::PipelineStageFlags2 src_stage_mask, dst_stage_mask;

  switch (transition) {
    case (layout_transition::dst_to_src):
      src_access_mask = vk::AccessFlagBits2::eTransferWrite;
      dst_access_mask = vk::AccessFlagBits2::eTransferRead;
      src_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;
      dst_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;
      break;
    case (layout_transition::src_to_shader_read):
      src_access_mask = vk::AccessFlagBits2::eTransferRead;
      dst_access_mask = vk::AccessFlagBits2::eShaderRead;
      src_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;
      dst_stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;
      break;
    case (layout_transition::dst_to_shader_read):
      src_access_mask = vk::AccessFlagBits2::eTransferWrite;
      dst_access_mask = vk::AccessFlagBits2::eShaderRead;
      src_stage_mask  = vk::PipelineStageFlagBits2::eTransfer;
      dst_stage_mask  = vk::PipelineStageFlagBits2::eFragmentShader;
      break;
    default:
      throw std::runtime_error("Layout transition not implemented");
  }

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
          .baseMipLevel   = mip_level,
          .levelCount     = mip_level_count,
          .baseArrayLayer = 0,
          .layerCount     = 1
      }
  };
  vk::DependencyInfo dependency_info = {
      .dependencyFlags = {}, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier
  };
  command_buffer.pipelineBarrier2(dependency_info);
}

void transition_image_global_layout(
    gpu_image& image,
    vk::raii::CommandBuffer& command_buffer,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask
) {
  if (!image.layout.has_value()) {
    throw std::runtime_error(
        "Tried to transition image layout using transition_image_global_texture but the image's "
        "mip levels don't have the same layout"
    );
  }
  transition_image_layout(
      image.image,
      command_buffer,
      image.layout.value(),
      new_layout,
      src_access_mask,
      dst_access_mask,
      src_stage_mask,
      dst_stage_mask,
      image.aspect,
      0,
      image.mip_level_count
  );
  image.layout = new_layout;
}
void transition_image_global_layout(
    gpu_image& image,
    vk::raii::CommandBuffer& command_buffer,
    layout_transition transition
) {
  if (!image.layout.has_value()) {
    throw std::runtime_error(
        "Tried to transition image layout using transition_image_global_texture but the image's "
        "mip levels don't have the same layout"
    );
  }
  auto [old_layout, new_layout] = associated_transition_layouts.at(transition);
  if (old_layout != image.layout.value()) {
    throw std::runtime_error("Given layout transition doesn't match src texture's layout");
  }
  transition_image_layout(
      image.image, command_buffer, transition, image.aspect, 0, image.mip_level_count
  );
  image.layout = new_layout;
}

void transition_image_mip_layout(
    gpu_image& image,
    vk::raii::CommandBuffer& command_buffer,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    uint32_t mip_level
) {
  transition_image_layout(
      image.image,
      command_buffer,
      image.layout.value(),
      new_layout,
      src_access_mask,
      dst_access_mask,
      src_stage_mask,
      dst_stage_mask,
      image.aspect,
      mip_level
  );
  image.layout.reset();
}

void transition_image_mip_layout(
    gpu_image& image,
    vk::raii::CommandBuffer& command_buffer,
    layout_transition transition,
    uint32_t mip_level
) {
  transition_image_layout(image.image, command_buffer, transition, image.aspect, mip_level);
  image.layout.reset();
}