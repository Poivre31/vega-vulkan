#pragma once

#include "graphics.hpp"

struct gpu_image {
  vma::raii::Image image   = nullptr;
  vk::raii::ImageView view = nullptr;
};

struct gpu_material {
  glm::vec3 color{};
  uint32_t texture_id{};
};
