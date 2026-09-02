#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "graphics.hpp"
#include "vk_mem_alloc_raii.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

struct gpu_image {
  // gpu_image(std::nullptr_t) {}
  // gpu_image(
  //     vma::raii::Image&& image,
  //     vk::raii::ImageView&& view,
  //     uint32_t width,
  //     uint32_t height,
  //     vk::Format format,
  //     vk::ImageAspectFlags aspect,
  //     uint32_t mip_level_count,
  //     vk::raii::Sampler* sampler = nullptr
  // )
  //     : image(std::move(image)),
  //       view(std::move(view)),
  //       width(width),
  //       height(height),
  //       format(format),
  //       aspect(aspect),
  //       mip_level_count(mip_level_count),
  //       sampler(sampler) {}

  vma::raii::Image image   = nullptr;
  vk::raii::ImageView view = nullptr;
  uint32_t width{};
  uint32_t height{};
  vk::Format format{};
  vk::ImageAspectFlags aspect;
  std::optional<vk::ImageLayout> layout = vk::ImageLayout::eUndefined;

  uint32_t mip_level_count = 1;

  vk::raii::Sampler* sampler = nullptr;
};

struct gpu_material {
  glm::vec4 color{};
  uint32_t texture_id{};
};
