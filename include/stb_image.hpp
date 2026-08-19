#pragma once

#include <stb_image.h>  // IWYU pragma: export

#include <filesystem>
#include <iostream>

enum class stbChannels : uint8_t { RGB = STBI_rgb, RGBA = STBI_rgb_alpha };

class stb_image {
 public:
  stb_image() = default;
  stb_image(const std::string& path, stbChannels target_channels = stbChannels::RGBA) {
    load(path, target_channels);
  }

  ~stb_image() {
    if (_data) {
      stbi_image_free(_data);
    }
  }

  stb_image(stb_image&)  = default;
  stb_image(stb_image&&) = default;

  stb_image& operator=(const stb_image&) = default;
  stb_image& operator=(stb_image&&)      = default;

  void load(std::string path, stbChannels target_channels = stbChannels::RGBA) {
    if (_data) {
      stbi_image_free(_data);
      _data = nullptr;
    }

    if (!std::filesystem::exists(path)) {
      if (std::filesystem::exists("ressources/textures/" + path)) {
        path = "ressources/textures/" + path;
      } else {
        std::cerr << "Image " << path << " doesn't exist in " << std::filesystem::current_path()
                  << " or " << std::filesystem::absolute("ressources/textures/");
        std::exit(1);
      }
    }

    _data = stbi_load(
        path.data(), &_tex_width, &_tex_height, &_channels, static_cast<int>(target_channels)
    );
    if (!_data) {
      std::cerr << "Image " << path << " exists but loading failed\n";
      std::exit(1);
    }
    _channels = static_cast<int>(target_channels);
  }

  [[nodiscard]] stbi_uc* data() const { return _data; }

  [[nodiscard]] int width() const { return _tex_width; }
  [[nodiscard]] int height() const { return _tex_height; }
  [[nodiscard]] int channels() const { return _channels; }

 private:
  int _tex_width  = 0;
  int _tex_height = 0;
  int _channels   = 0;
  stbi_uc* _data  = nullptr;
};