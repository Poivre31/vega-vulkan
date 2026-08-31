#pragma once

#include "allocator.hpp"
#include "image.hpp"
#include "layer.hpp"
#include "object_loader.hpp"
#include "stb_image.hpp"

class material {
 public:
  material() = default;
  material(
      application_context* context,
      const std::string& parent_directory,
      tinyobj::material_t material
  ) {
    stb_image image;
    if (!material.diffuse_texname.empty()) {
      image = stb_image(parent_directory + material.diffuse_texname);
    } else {
      auto color = glm::vec4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1);
      console::get(consoles::assets)
          ->warn(
              "No texture specified for material {}, replacing by 1x1 texture of color "
              "({},{},{})",
              material.name,
              color.r,
              color.g,
              color.b
          );
      image = stb_image(color, 1, 1);
    }

    albedo = context->resources.textures_.push(
        std::move(load_texture_to_gpu(context->vulkan, std::move(image)))
    );
  }

 private:
  handle<gpu_image> albedo;
};