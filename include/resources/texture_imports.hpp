#pragma once

enum class channels : uint8_t { RGB = 3, RGBA = 4 };

struct texture_info {
  const char* texture_path{};
  bool enable_mipmaps      = true;
  bool is_srgb             = true;
  channels target_channels = channels::RGBA;
};

namespace textures {

const texture_info beer{
    .texture_path   = "resources/textures/beer.png",
    .enable_mipmaps = false,
};

const texture_info viking{
    .texture_path   = "resources/textures/viking_room.png",
    .enable_mipmaps = true,
};

}  // namespace textures