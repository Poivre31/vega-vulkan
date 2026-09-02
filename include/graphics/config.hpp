#pragma once

#include "graphics.hpp"

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

constexpr uint8_t max_number_of_exception_error = 10;

const std::vector<const char*> requested_extensions = {};
const std::vector<const char*> requested_layers     = {
    "VK_LAYER_KHRONOS_validation",
};
const std::vector<const char*> requested_device_extensions = {
    vk::KHRSwapchainExtensionName,  //, vk::EXTPageableDeviceLocalMemoryExtensionName
                                    // vk::ARMTensorsExtensionName,
};

constexpr auto color_format = vk::Format::eB8G8R8A8Srgb;
constexpr auto color_space  = vk::ColorSpaceKHR::eSrgbNonlinear;

constexpr uint32_t target_swapchain_image_count = 3;

constexpr bool vsync = false;

const std::string shader_path = "resources/shaders/lit_shader.spv";

constexpr uint32_t max_number_of_textures = 1024;

constexpr uint32_t frames_in_flight  = 2;
constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.9F, 0.1F, 0.2F, 1.F);
constexpr vk::ClearValue clear_depth = vk::ClearDepthStencilValue(1.F, 0);

constexpr vk::SampleCountFlags target_msaa_sample_count = vk::SampleCountFlagBits::e4;
constexpr float msaa_shading_rate                       = 0.5F;

}  // namespace vulkan_config