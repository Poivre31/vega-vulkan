#pragma once

#include <stb_image.h>  // IWYU pragma: export

#include <filesystem>
#include <memory>
#include <console/console.hpp>

enum class stb_channels : uint8_t { RGB = STBI_rgb, RGBA = STBI_rgb_alpha };

using stb_image_ptr = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;

constexpr size_t fallback_image_width    = 2;
constexpr size_t fallback_image_height   = 2;
constexpr size_t fallback_image_channels = 4;

constexpr std::array<stbi_uc, fallback_image_channels> fallback_image_color_1 =
    {0xFF, 0, 0xFF, 0xFF};
constexpr std::array<stbi_uc, fallback_image_channels> fallback_image_color_2 =
    {0x4F, 0, 0x4F, 0xFF};

/**
 * @brief A wrapped class around stbi_uc* to help load 8B color images using stb image loader. Owns
 * the memory and free it at destruction/new image load
 *
 */
class stb_image {
 public:
  stb_image() = default;

  /**
   * @brief Tries to load image using stb image loader. Loads a fallback texture on failure.
   *
   * @param image_path Target path relative to binary folder or resources/textures/
   * @param target_channels The channels you want in the loaded image, either RGB or RGBA
   *
   */
  void load(std::string image_path, stb_channels target_channels = stb_channels::RGBA) noexcept {
    if (_image_loaded) {
      unload();
    }

    try {
      auto path = std::filesystem::path(image_path);
      if (!std::filesystem::exists(path)) {
        path = std::filesystem::path("resources/textures/").append(image_path);
        if (!std::filesystem::exists(path)) {
          console::get(consoles::assets)
              ->error(
                  "Image '{}' doesn't exist in directory '{}' or '{}', loading fallback texture",
                  image_path,
                  std::filesystem::current_path().string(),
                  std::filesystem::absolute("resources/textures/").string()
              );
          return;
        }
      }
      auto* image_data = stbi_load(
          path.string().data(),
          &_tex_width,
          &_tex_height,
          &_channels,
          static_cast<int>(target_channels)
      );
      if (!image_data) {
        console::get(consoles::assets, true)
            ->error(
                "Image '{:s}' exists but loading failed,  loading fallback texture", path.string()
            );
        return;
      }
      _data         = stb_image_ptr(image_data, stbi_image_free);
      _channels     = static_cast<int>(target_channels);
      _image_loaded = true;
    } catch (const std::filesystem::filesystem_error& e) {
      console::get(consoles::assets)
          ->error("Filesystem error loading '{}': {}", image_path, e.what());
    } catch (const std::exception& e) {
      console::get(consoles::assets)
          ->error("Unexpected error loading '{}': {}", image_path, e.what());
    }
  }

  /** @brief Unloads loaded image if there is one, otherwise output warning and do nothing.*/
  void unload() noexcept {
    if (!_image_loaded) {
      console::get(consoles::assets)
          ->warn("Trying to unload image but no image has been loaded yet.\nDoing nothing");
    } else {
      _data.reset();
      _image_loaded = false;
    }
  }

  /** @brief Get the image memory pointer after image has been loaded, returns nullptr if it hasn't
   * been loaded */
  [[nodiscard]] stbi_uc* data() const noexcept {
    if (!_image_loaded) {
      console::get(consoles::assets)
          ->error(
              "Accessing an image whose data hasn't been loaded yet, returning fallback texture"
          );
      return _fallback_image.data()->data();
    } else {
      return _data.get();
    }
  }

  [[nodiscard]] int width() const noexcept {
    return _image_loaded ? _tex_width : fallback_image_width;
  }
  [[nodiscard]] int height() const noexcept {
    return _image_loaded ? _tex_height : fallback_image_height;
  }
  /** Number of channels in the loaded image, either 3 (RGB) or 4 (RGBA) */
  [[nodiscard]] int channels() const noexcept {
    return _image_loaded ? _channels : fallback_image_channels;
  }
  [[nodiscard]] int pitch() const noexcept { return width() * channels(); }

 private:
  stb_image_ptr _data{nullptr, &stbi_image_free};
  int _tex_width{};
  int _tex_height{};
  int _channels{};
  bool _image_loaded = false;

  static inline std::array<
      std::array<stbi_uc, fallback_image_channels>,
      fallback_image_width * fallback_image_height>
      _fallback_image{
          fallback_image_color_1,
          fallback_image_color_2,
          fallback_image_color_2,
          fallback_image_color_1
      };
};