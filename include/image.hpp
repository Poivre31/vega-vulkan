#pragma once

#include "graphics.hpp"

struct gpu_image {
  vma::raii::Image image   = nullptr;
  vk::raii::ImageView view = nullptr;
};

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