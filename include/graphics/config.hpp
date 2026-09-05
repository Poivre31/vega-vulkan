#pragma once

#include "graphics.hpp"
#include "vulkan/vulkan.hpp"

struct dynamic_config {
  vk::Format color_format       = vk::Format::eB8G8R8A8Unorm;
  vk::ColorSpaceKHR color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
  vk::Format depth_format       = vk::Format::eD32Sfloat;

  uint32_t min_swapchain_image_count{};
  uint32_t max_swapchain_image_count{};
  uint32_t swapchain_image_count = 3;
  bool vsync                     = true;

  vk::ClearValue clear_color = vk::ClearColorValue(0.F, 0.F, 0.F, 0.F);
  vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.F, 0);

  vk::SampleCountFlagBits msaa_sample_count = vk::SampleCountFlagBits::e4;
  std::vector<vk::SampleCountFlagBits> available_msaa_sample_counts;
  float msaa_shading_rate = 0.3F;
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

// constexpr auto color_format = vk::Format::eB8G8R8A8Unorm;
// constexpr auto color_space  = vk::ColorSpaceKHR::eSrgbNonlinear;
// constexpr auto depth_format = vk::Format::eD32Sfloat;

// struct {
//   vk::Format color_format               = vk::Format::eB8G8R8A8Unorm;
//   vk::ColorSpaceKHR color_space         = vk::ColorSpaceKHR::eSrgbNonlinear;
//   uint32_t target_swapchain_image_count = 3;
//   bool vsync                            = true;
// } swapchain;

// struct {
//   vk::Format depth_format = vk::Format::eD32Sfloat;
// } graphics_pipeline;

// constexpr uint32_t target_swapchain_image_count = 3;

const std::string shader_path = "resources/shaders/lit_shader.spv";

constexpr uint32_t max_number_of_textures = 512;

constexpr uint32_t frames_in_flight = 2;
// constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.F, 0.F, 0.F, 0.F);
// constexpr vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.F, 0);

// constexpr vk::SampleCountFlags target_msaa_sample_count = vk::SampleCountFlagBits::e4;
// constexpr float msaa_shading_rate                       = 0.3F;

}  // namespace vulkan_config