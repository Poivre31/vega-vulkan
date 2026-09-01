#pragma once

#include <optional>

#include "graphics.hpp"

struct gpu_image {
  vma::raii::Image image   = nullptr;
  vk::raii::ImageView view = nullptr;
  uint32_t width;
  uint32_t height;
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
