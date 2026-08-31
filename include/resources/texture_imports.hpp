#pragma once

#include "stb_image.hpp"
#include "image.hpp"
#include "layer.hpp"

struct texture_info {
  const char* texture_path{};
  bool enable_mipmaps = true;
  bool is_srgb        = true;
};

std::vector<handle<gpu_image>>
upload_textures(application_context* context, std::vector<stb_image>&& cpu_textures) {
  auto textures = std::move(cpu_textures);
  std::vector<handle<gpu_image>> indices;
  for (auto& image : textures) {
    auto handle = context->resources.textures_.push(
        std::move(load_texture_to_gpu(context->vulkan, std::move(image)))
    );
    indices.push_back(handle);
  }
  return indices;
}

namespace textures {

const texture_info beer{
    .texture_path   = "resources/textures/beer.png",
    .enable_mipmaps = false,  // MOVE TO SAMPLER SETTING
};

}  // namespace textures