#pragma once

struct vulkan_context {
  vk::raii::PhysicalDevice* physical_device = nullptr;
  uint32_t graphics_queue_family            = ~0;
  vk::raii::Queue* graphics_queue           = nullptr;
  vk::raii::Device* device                  = nullptr;
  vma::raii::Allocator* allocator           = nullptr;
  vk::raii::CommandPool* command_pool       = nullptr;

  vk::raii::DescriptorSets* descriptor_sets = nullptr;
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

constexpr uint8_t max_number_of_exception_error = 10;

const std::vector<const char*> requested_extensions = {};
const std::vector<const char*> requested_layers = {"VK_LAYER_LUNARG_monitor"};  // VK_LAYER_KHRONOS_validation
                                                                                // handled elsewhere
const std::vector<const char*> requested_device_extensions = {
    vk::KHRSwapchainExtensionName  //, vk::EXTPageableDeviceLocalMemoryExtensionName
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

}  // namespace vulkan_config