#pragma once

#include "graphics/gpu_objects.hpp"

#include "tiny_obj_loader.h"
#include "handle.hpp"

class material {
 public:
  material() = default;
  material(handle<gpu_image> albedo) : albedo_texture(albedo) {}
  material(glm::vec4 color) : albedo_color(color) {}
  material(glm::vec4 color, handle<gpu_image> albedo)
      : albedo_color(color), albedo_texture(albedo) {}

  void set_albedo_color(glm::vec4 color) { albedo_color = color; }
  [[nodiscard]] glm::vec4 get_albedo_color() const { return albedo_color; }
  [[nodiscard]] handle<gpu_image> get_albedo_texture() const { return albedo_texture; }

 private:
  handle<gpu_image> albedo_texture{};
  glm::vec4 albedo_color{};
};