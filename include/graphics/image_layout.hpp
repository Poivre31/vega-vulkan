#pragma once

#include <stdexcept>
#include "graphics.hpp"
#include "gpu_objects.hpp"
#include "vulkan/vulkan.hpp"

enum class layout_transition : uint8_t {
  dst_to_src,
  src_to_shader_read,
  dst_to_shader_read,
  undef_to_src,
  undef_to_dst,
  undef_to_color_attachment,
  undef_to_depth_attachment,
  undef_to_shader_storage_write,
};

struct layout_transition_data {
  vk::ImageLayout src_layout;
  vk::ImageLayout dst_layout;
  vk::AccessFlags2 src_access;
  vk::AccessFlags2 dst_access;
  vk::PipelineStageFlags2 src_stage;
  vk::PipelineStageFlags2 dst_stage;
};

const std::unordered_map<layout_transition, layout_transition_data> associated_transition_layouts{
    {layout_transition::dst_to_src,
     {
         .src_layout = vk::ImageLayout::eTransferDstOptimal,
         .dst_layout = vk::ImageLayout::eTransferSrcOptimal,
         .src_access = vk::AccessFlagBits2::eTransferWrite,
         .dst_access = vk::AccessFlagBits2::eTransferRead,
         .src_stage  = vk::PipelineStageFlagBits2::eTransfer,
         .dst_stage  = vk::PipelineStageFlagBits2::eTransfer,
     }},
    {layout_transition::src_to_shader_read,
     {
         .src_layout = vk::ImageLayout::eTransferSrcOptimal,
         .dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
         .src_access = vk::AccessFlagBits2::eTransferRead,
         .dst_access = vk::AccessFlagBits2::eShaderRead,
         .src_stage  = vk::PipelineStageFlagBits2::eTransfer,
         .dst_stage  = vk::PipelineStageFlagBits2::eFragmentShader,
     }},
    {layout_transition::dst_to_shader_read,
     {
         .src_layout = vk::ImageLayout::eTransferDstOptimal,
         .dst_layout = vk::ImageLayout::eShaderReadOnlyOptimal,
         .src_access = vk::AccessFlagBits2::eTransferWrite,
         .dst_access = vk::AccessFlagBits2::eShaderRead,
         .src_stage  = vk::PipelineStageFlagBits2::eTransfer,
         .dst_stage  = vk::PipelineStageFlagBits2::eFragmentShader,
     }},
    {layout_transition::undef_to_src,
     {
         .src_layout = vk::ImageLayout::eUndefined,
         .dst_layout = vk::ImageLayout::eTransferSrcOptimal,
         .src_access = {},
         .dst_access = vk::AccessFlagBits2::eTransferRead,
         .src_stage  = vk::PipelineStageFlagBits2::eTopOfPipe,
         .dst_stage  = vk::PipelineStageFlagBits2::eTransfer,
     }},
    {layout_transition::undef_to_dst,
     {
         .src_layout = vk::ImageLayout::eUndefined,
         .dst_layout = vk::ImageLayout::eTransferDstOptimal,
         .src_access = {},
         .dst_access = vk::AccessFlagBits2::eTransferWrite,
         .src_stage  = vk::PipelineStageFlagBits2::eTopOfPipe,
         .dst_stage  = vk::PipelineStageFlagBits2::eTransfer,
     }},
    {layout_transition::undef_to_color_attachment,
     {
         .src_layout = vk::ImageLayout::eUndefined,
         .dst_layout = vk::ImageLayout::eColorAttachmentOptimal,
         .src_access = {},
         .dst_access = vk::AccessFlagBits2::eColorAttachmentWrite,
         .src_stage  = vk::PipelineStageFlagBits2::eTopOfPipe,
         .dst_stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
     }},
    {layout_transition::undef_to_depth_attachment,
     {.src_layout = vk::ImageLayout::eUndefined,
      .dst_layout = vk::ImageLayout::eDepthAttachmentOptimal,
      .src_access = {},
      .dst_access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
      .src_stage  = vk::PipelineStageFlagBits2::eTopOfPipe,
      .dst_stage  = vk::PipelineStageFlagBits2::eEarlyFragmentTests
                    | vk::PipelineStageFlagBits2::eLateFragmentTests}},
    {layout_transition::undef_to_shader_storage_write,
     {
         .src_layout = vk::ImageLayout::eUndefined,
         .dst_layout = vk::ImageLayout::eGeneral,
         .src_access = {},
         .dst_access = vk::AccessFlagBits2::eShaderStorageWrite,
         .src_stage  = vk::PipelineStageFlagBits2::eTopOfPipe,
         .dst_stage  = vk::PipelineStageFlagBits2::eComputeShader,
     }},
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
  if (!associated_transition_layouts.contains(transition)) {
    throw std::runtime_error("Layout transition is not implemented");
  }
  auto transition_data = associated_transition_layouts.at(transition);

  vk::ImageMemoryBarrier2 barrier = {
      .srcStageMask        = transition_data.src_stage,
      .srcAccessMask       = transition_data.src_access,
      .dstStageMask        = transition_data.dst_stage,
      .dstAccessMask       = transition_data.dst_access,
      .oldLayout           = transition_data.src_layout,
      .newLayout           = transition_data.dst_layout,
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
  auto transition_data = associated_transition_layouts.at(transition);

  transition_image_layout(
      image.image, command_buffer, transition, image.aspect, 0, image.mip_level_count
  );
  image.layout = transition_data.dst_layout;
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