#pragma once

#include <vector>
#include "graphics.hpp"
#include "vulkan/vulkan.hpp"

struct dynamic_config {
  vk::Format present_color_format       = vk::Format::eB8G8R8A8Unorm;
  vk::ColorSpaceKHR present_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
  vk::Format depth_format               = vk::Format::eD32Sfloat;
  vk::Format raster_color_format        = vk::Format::eR16G16B16A16Sfloat;
  std::vector<vk::Format> available_raster_format{
      vk::Format::eB8G8R8A8Unorm,
      vk::Format::eA2B10G10R10UnormPack32,
      vk::Format::eB10G11R11UfloatPack32,
      vk::Format::eR16G16B16A16Sfloat,
  };

  uint32_t min_swapchain_image_count{};
  uint32_t max_swapchain_image_count{};
  uint32_t swapchain_image_count = 3;
  bool vsync                     = true;

  vk::ClearValue clear_color = vk::ClearColorValue(0.F, 0.F, 0.F, 0.F);
  vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.F, 0);

  vk::SampleCountFlagBits msaa_sample_count = vk::SampleCountFlagBits::e4;
  std::vector<vk::SampleCountFlagBits> available_msaa_sample_counts;
  float msaa_shading_rate = 0.3F;

  bool enable_post_processing = true;
  struct {
    float dithering_stength = 0.2F;

    int blur_kernel_radius = 0;
    float blur_strength    = 2.F;

    float exposure      = 1.F;
    bool srgb_transform = true;
    float srgb_gamma    = 2.4F;
    float srgb_offset   = 0.055F;
  } post_processing;
};

namespace vulkan_config {

const vk::ApplicationInfo vulkan_info{
    .pApplicationName   = "Vega Vulkan",
    .applicationVersion = vk::makeVersion(0, 1, 0),
    .apiVersion         = vk::ApiVersion13
};

#ifdef NDEBUG
constexpr bool enable_validation = false;
#else
constexpr bool enable_validation = true;
#endif

const std::vector<const char*> requested_extensions        = {};
const std::vector<const char*> requested_layers            = {};
const std::vector<const char*> requested_device_extensions = {
    vk::KHRSwapchainExtensionName,  //, vk::EXTPageableDeviceLocalMemoryExtensionName
    vk::EXTRobustness2ExtensionName,
};

const std::string shader_path = "resources/shaders/lit_shader.spv";

constexpr uint32_t max_number_of_textures = 512;

constexpr uint32_t frames_in_flight = 2;

}  // namespace vulkan_config