#pragma once
#include "layer.hpp"
#include "graphics.hpp"
#include <algorithm>
#include <console/console.hpp>
#include <SDL3/SDL_vulkan.h>
#include "sdl.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"
#include "vulkan_context.hpp"
#include <io.hpp>
#include "image.hpp"

inline bool is_device_suitable(const vk::raii::PhysicalDevice& device) {
  auto device_properties = device.getProperties();
  auto device_features   = device.getFeatures();

  if (device_properties.apiVersion < vk::ApiVersion13) {
    return false;
  }

  auto available_queue_families = device.getQueueFamilyProperties();
  if (std::none_of(// NOLINT
          available_queue_families.begin(),
          available_queue_families.end(),
          [](auto const& queue_properties) {
            return queue_properties.queueFlags & vk::QueueFlagBits::eGraphics;
          }
      )) {
    return false;
  }

  auto available_device_extensions = device.enumerateDeviceExtensionProperties();

  for (const auto* extension : vulkan_config::requested_device_extensions) {
    if (std::ranges::none_of(
            available_device_extensions, [extension](auto const& avaibleExtension) {
              return strcmp(avaibleExtension.extensionName, extension) == 0;
            }
        )) {
      return false;
    }
  }

  return true;
}

inline vk::SurfaceFormatKHR get_swapchain_format(
    const vk::raii::PhysicalDevice& physical_device,
    const vk::raii::SurfaceKHR& surface
) {
  auto available = physical_device.getSurfaceFormatsKHR(*surface);

  std::string error_message("Surface format choosed is not available, choose among: \n");
  for (const auto& format : available) {
    if (format.format == vulkan_config::color_format
        && format.colorSpace == vulkan_config::color_space) {
      return format;
    }
    error_message += "  -" + vk::to_string(format.format) + " " + vk::to_string(format.colorSpace)
                     + "\n";
  }

  throw std::runtime_error(error_message);
}

inline vk::Extent2D
get_swapchain_extent(const vk::SurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  }

  int width = 0, height = 0;
  SDL_GetWindowSizeInPixels(window, &width, &height);

  return {
      .width = std::clamp(
          static_cast<uint32_t>(width),
          capabilities.minImageExtent.width,
          capabilities.maxImageExtent.width
      ),
      .height = std::clamp(
          static_cast<uint32_t>(height),
          capabilities.minImageExtent.height,
          capabilities.maxImageExtent.height
      )
  };
}

inline vk::PresentModeKHR
get_present_mode(const vk::PhysicalDevice& physical_device, const vk::raii::SurfaceKHR& surface) {
  auto available = physical_device.getSurfacePresentModesKHR(*surface);
  if (!vulkan_config::vsync) {
    for (const auto& present_mode : available) {
      if (present_mode == vk::PresentModeKHR::eMailbox) {
        return vk::PresentModeKHR::eMailbox;
      }
    }
    for (const auto& present_mode : available) {
      if (present_mode == vk::PresentModeKHR::eImmediate) {
        return vk::PresentModeKHR::eImmediate;
      }
    }
  }
  return vk::PresentModeKHR::eFifo;
}

inline vk::raii::ShaderModule
create_shader_module(const vk::raii::Device& device, std::string shader_code) {
  vk::ShaderModuleCreateInfo shader_module_info{
      .codeSize = static_cast<uint32_t>(shader_code.size()),
      .pCode    = reinterpret_cast<uint32_t*>(shader_code.data())
  };
  return {device, shader_module_info};
}

struct PushConstants {
  glm::mat4x4 view_projection_matrix{};
  float time{};
};

class vulkan_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;

  bool init() noexcept final {
    try {
      create_instance();
      setup_console_callback();
      create_surface();
      pick_physical_device();
      create_logical_device();
      initialize_vma();
      create_swapchain();
      create_descriptor_set_layouts();
      create_graphics_pipeline();
      create_command_pool();
      create_depth_resources();
      create_texture_sampler();
      create_descriptor_pools();
      create_descriptor_sets();
      create_command_buffer();
      create_synchronisation_objects();
    } catch (...) {
      handle_exception("initialisation");
      return false;
    }
    _initialised = true;

    get_app_context()->vulkan = {
        .physical_device       = &_physical_device,
        .graphics_queue_family = _graphics_queue_family,
        .graphics_queue        = &_graphics_queue,
        .device                = &_device,
        .allocator             = &_allocator,
        .command_pool          = &_command_pool,
        .descriptor_sets       = &_descriptor_sets,
    };
    _console->info("Vulkan was successfully initialised");

    return true;
  }

  void update(double dt) noexcept final {
    if (!_initialised) {
      return;
    }
    try {
      if (get_app_context()->vulkan.recreate_graphics_pipeline) {
        recreate_graphics_pipeline();
        get_app_context()->vulkan.recreate_graphics_pipeline = false;
      }
      if (get_app_context()->active_camera) {
        _push_constants.view_projection_matrix =
            get_app_context()->active_camera->get_view_projection_matrix();
      } else {
        throw std::runtime_error("No active camera set");
        _push_constants.view_projection_matrix = glm::mat4x4{1.F};
      }
      _push_constants.time = static_cast<float>(timer::get_elapsed_time());
      draw_frame();
    } catch (...) {
      handle_exception("update");
    }
  }

  void cleanup() noexcept final {
    if (*_device) {
      _device.waitIdle();
    }
  }

  static VKAPI_ATTR vk::Bool32 VKAPI_CALL debug_callback(
      vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
      vk::DebugUtilsMessageTypeFlagsEXT type,
      const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
      void* pUserData
  ) {
    switch (severity) {
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        _validation_console->warn(
            "Validation layer: type {} : {}", to_string(type), pCallbackData->pMessage
        );
        break;
      case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        _validation_console->error(
            "Validation layer: type {} : {}", to_string(type), pCallbackData->pMessage
        );
        break;
      default:
        break;
    }
    return vk::False;
  }

 private:
  // [[nodiscard]] bool check_number_of_messages(std::string_view message) {
  //   uint64_t message_hash           = std::hash<std::string_view>{}(message);
  //   auto number_of_message_instance = _debug_message_instances.find(message_hash);
  //   if (number_of_message_instance != _debug_message_instances.end()) {
  //     if (number_of_message_instance->second > vulkan_config::max_number_of_exception_error) {
  //       return false;
  //     } else {
  //       if (number_of_message_instance->second == vulkan_config::max_number_of_exception_error) {
  //         _console->warn(
  //             "Exception has been shown {}, this is the last time it is printed:  ",
  //             vulkan_config::max_number_of_exception_error
  //         );
  //       }
  //       number_of_message_instance->second++;
  //     }
  //   } else {
  //     _debug_message_instances.emplace(message_hash, 0);
  //   }
  //   return true;
  // }

  static void handle_exception(const std::string& context) noexcept {
    try {
      throw;
    } catch (const std::exception& e) {
      _console->error("Exception during Vulkan {} : {}", context, e.what());
    } catch (...) {
      _console->error("Unknown error during Vulkan {}", context);
    }
  }

  void create_instance() {
    if (_instance_running) {
      _console->error(
          "A Vulkan instance has already been created but Vega can only handle one at a time. "
          "Returning from initialisation."
      );
      return;
    }

    _context = vk::raii::Context();

    // SELECTING EXTENSIONS
    uint32_t extensions_count       = 0;
    const auto* required_extensions = SDL_Vulkan_GetInstanceExtensions(&extensions_count);

    std::vector<const char*> extensions = vulkan_config::requested_extensions;
    extensions.insert(
        extensions.end(), required_extensions, required_extensions + extensions_count
    );
    if (vulkan_config::enable_validation) {
      extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    auto extensions_properties = _context.enumerateInstanceExtensionProperties();
    for (const auto* extension : extensions) {
      if (std::ranges::none_of(
              extensions_properties, [extension](vk::ExtensionProperties extension_property) {
                return std::strcmp(extension_property.extensionName, extension) == 0;
              }
          )) {
        throw std::runtime_error(
            fmt::format("Requested extension '{}' is not supported, exiting", extension)
        );
      }
    }

    // SELECTING LAYERS
    std::vector<const char*> layers;
    if (vulkan_config::enable_validation) {
      layers = vulkan_config::requested_layers;
    }

    auto layer_properties = _context.enumerateInstanceLayerProperties();
    size_t enabled_layers = layers.size();
    for (int i = 0; i < enabled_layers;) {
      const auto* layer = layers[i];
      if (std::ranges::none_of(layer_properties, [layer](vk::LayerProperties layer_property) {
            return std::strcmp(layer_property.layerName, layer) == 0;
          })) {
        _console->error("Requested layer '{}' is not supported, disabling it", layer);
        layers.erase(layers.begin() + i);
        enabled_layers--;
      } else {
        i++;
      }
    }

    // CREATING INSTANCE
    vk::InstanceCreateInfo instance_info{
        .pApplicationInfo        = &vulkan_config::vulkan_info,
        .enabledLayerCount       = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames     = layers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    _instance = vk::raii::Instance(_context, instance_info);

    _instance_running = true;
  }

  void setup_console_callback() {
    // SETING UP CONSOLE CALLBACK
    if (!vulkan_config::enable_validation) {
      return;
    }
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
    );
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType     = messageTypeFlags,
        .pfnUserCallback = &debug_callback
    };
    _debug_messenger = _instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
  }

  void create_surface() {
    // CREATING SDL/VULKAN SURFACE
    VkSurfaceKHR surface{};
    if (!SDL_Vulkan_CreateSurface(get_app_context()->window, *_instance, nullptr, &surface)) {
      throw sdl_exception(SDL_GetError());
    }
    _surface = vk::raii::SurfaceKHR(_instance, surface);
  }

  void pick_physical_device() {
    // FINDING PHYSICAL DEVICES
    auto available = _instance.enumeratePhysicalDevices();
    if (available.size() == 0) {
      throw std::runtime_error("No physical device found");
    }
    std::vector<vk::raii::PhysicalDevice> suitable;
    for (const auto& physical_device : available) {
      if (is_device_suitable(physical_device)) {
        suitable.push_back(physical_device);
      }
    }
    if (suitable.size() == 0) {
      throw std::runtime_error(
          "No physical device found is suitable (might be because it doesn't support required "
          "vulkan api version, has no graphics queue or extensions such as 'swapchain KHR' are not "
          "available), exiting"
      );
    }
    if (suitable.size() > 1) {
      _console->info(
          "Multiple suitable physical devices available, picking the first one, discrete one in "
          "priority"
      );
      _console->warn("TO DO: improve physical device selection");
    }

    // SELECTING ONE BASE ON ITS PROPERTIES
    for (const auto& physical_device : suitable) {
      if (physical_device.getProperties2().properties.deviceType
          == vk::PhysicalDeviceType::eDiscreteGpu) {
        _physical_device = physical_device;
        break;
      }
    }
    if (!*_physical_device) {
      _physical_device = suitable.front();
    }

    auto device_properties = _physical_device.getProperties2().properties;
    _console->info(
        "Selected device [{}] of type [{}]",
        device_properties.deviceName.data(),
        vk::to_string(device_properties.deviceType)
    );

    auto features = _physical_device.template getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

    if (!features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy) {
      throw std::runtime_error("Selected physical device doesn't support 'anisotropy', exiting");
    }
    if (!features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters) {
      throw std::runtime_error(
          "Selected physical device doesn't support 'shader draw parameters', exiting"
      );
    }
    if (!features.template get<vk::PhysicalDeviceVulkan12Features>()
             .shaderSampledImageArrayNonUniformIndexing) {
      throw std::runtime_error(
          "Selected physical device doesn't support 'sampled image array non uniform indexing', "
          "exiting"
      );
    }
    if (!features.template get<vk::PhysicalDeviceVulkan12Features>().runtimeDescriptorArray) {
      throw std::runtime_error(
          "Selected physical device doesn't support 'runtime descriptor array', exiting"
      );
    }
    if (!features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering) {
      throw std::runtime_error(
          "Selected physical device doesn't support 'dynamic rendering', exiting"
      );
    }
    if (!features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2) {
      throw std::runtime_error(
          "Selected physical device doesn't support 'synchronization 2', exiting"
      );
    }
    if (!features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>()
             .extendedDynamicState) {
      throw std::runtime_error(
          "Selected physical device doesn't support 'extended dynamic state', exiting"
      );
    }

    if (device_properties.limits.maxDescriptorSetSamplers < vulkan_config::max_number_of_textures) {
      throw std::runtime_error(
          fmt::format(
              "Selected physical device supports {} per stage descriptor samplers but reserved {} "
              "texture slots, exiting",
              device_properties.limits.maxDescriptorSetSamplers,
              vulkan_config::max_number_of_textures
          )
      );
    }
  }

  void create_logical_device() {
    // ENABLING FEATURES
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {
            {.features = {.samplerAnisotropy = vk::True}},
            {.shaderDrawParameters = vk::True},
            {
                .shaderSampledImageArrayNonUniformIndexing = vk::True,
                .runtimeDescriptorArray                    = vk::True,
            },
            {
                .synchronization2 = vk::True,
                .dynamicRendering = vk::True,
            },
            {.extendedDynamicState = vk::False}
        };

    // CHOOSING QUEUE FAMILY
    auto queue_family_properties = _physical_device.getQueueFamilyProperties2();
    uint32_t family_index        = 0;
    for (const auto& queue_family : queue_family_properties) {
      if ((queue_family.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)
          && _physical_device.getSurfaceSupportKHR(family_index, *_surface)) {
        _graphics_queue_family = family_index;
        break;
      }
      family_index++;
    }
    if (_graphics_queue_family == ~0) {
      throw std::runtime_error("No graphics family queue supporting SDL surface exists");
    }

    // CREATING GRAPHICS QUEUE
    float graphics_queue_priority = 0.5F;
    vk::DeviceQueueCreateInfo queue_info{
        .queueFamilyIndex = _graphics_queue_family,
        .queueCount       = 1,
        .pQueuePriorities = &graphics_queue_priority
    };

    // CREATING LOGICAL DEVICE
    vk::DeviceCreateInfo device_info{
        .pNext                = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos    = &queue_info,
        .enabledExtensionCount =
            static_cast<uint32_t>(vulkan_config::requested_device_extensions.size()),
        .ppEnabledExtensionNames = vulkan_config::requested_device_extensions.data(),
    };

    _device         = vk::raii::Device(_physical_device, device_info);
    _graphics_queue = vk::raii::Queue(_device, _graphics_queue_family, 0);
  }

  void initialize_vma() {
    vma::AllocatorCreateInfo allocator_info{.physicalDevice = _physical_device};
    _allocator = vma::raii::createAllocator(_instance, _device, allocator_info);
  }

  void create_swapchain() {
    // CREATING SWAPCHAIN
    auto capabilities = _physical_device.getSurfaceCapabilitiesKHR(*_surface);
    _swapchain_format = get_swapchain_format(_physical_device, _surface);
    _swapchain_extent = get_swapchain_extent(capabilities, get_app_context()->window);

    uint32_t requested_image_count = std::max(
        vulkan_config::target_swapchain_image_count, capabilities.minImageCount
    );
    if (capabilities.maxImageCount != 0) {
      requested_image_count = std::min(requested_image_count, capabilities.maxImageCount);
    }

    auto present_mode = get_present_mode(_physical_device, _surface);
    if (present_mode == vk::PresentModeKHR::eImmediate) {
      _console->warn(
          "Vsync is disabled and MailBox presentation mode is not available, choosed Immediate "
          "which may cause tearing"
      );
    }

    vk::SwapchainCreateInfoKHR swapchain_info{
        .surface          = *_surface,
        .minImageCount    = requested_image_count,
        .imageFormat      = _swapchain_format.format,
        .imageColorSpace  = _swapchain_format.colorSpace,
        .imageExtent      = _swapchain_extent,
        .imageArrayLayers = 1,  // Always 1 except for stereo 3D
        .imageUsage = vk::ImageUsageFlags::BitsType::eColorAttachment,  // To write directly to
                                                                        // the screen,
                                                                        // eTransferDst to
                                                                        // postProcess then send
                                                                        // to the screen
        .imageSharingMode = vk::SharingMode::eExclusive,    // One family queue write to the image
                                                            // at the time
        .preTransform     = capabilities.currentTransform,  // Like image rotation or flip
        .compositeAlpha   = vk::CompositeAlphaFlagsKHR::BitsType::eOpaque,
        .presentMode      = present_mode,
        .clipped          = vk::True,
        .oldSwapchain     = nullptr
    };

    // CREATING SWAPCHAIN IMAGE VIEWS
    _swapchain        = vk::raii::SwapchainKHR(_device, swapchain_info);
    _swapchain_images = _swapchain.getImages();

    vk::ImageViewCreateInfo view_info{
        .viewType         = vk::ImageViewType::e2D,
        .format           = _swapchain_format.format,
        .subresourceRange = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };

    for (const auto& image : _swapchain_images) {
      view_info.setImage(image);
      _swapchain_views.emplace_back(_device, view_info);

      vk::SemaphoreCreateInfo semaphore_info{};
      _swapchain_semaphores.emplace_back(_device, semaphore_info);
    }
  }

  void recreate_swapchain() {
    _console->info("Recreating swapchain");
    _device.waitIdle();
    _swapchain = nullptr;
    _swapchain_semaphores.clear();
    _swapchain_views.clear();
    _swapchain_images.clear();
    create_swapchain();
    create_depth_resources();
  }

  void create_descriptor_set_layouts() {
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings{vk::DescriptorSetLayoutBinding{
        .binding         = 0,
        .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = vulkan_config::max_number_of_textures,
        .stageFlags      = vk::ShaderStageFlagBits::eFragment
    }};

    // std::vector<vk::DescriptorBindingFlags> flags{vk::DescriptorBindingFlags{
    //     vk::DescriptorBindingFlagBits::ePartiallyBound
    //     | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    // }};

    // vk::DescriptorSetLayoutBindingFlagsCreateInfo flags_info{
    //     .bindingCount = static_cast<uint32_t>(flags.size()), .pBindingFlags = flags.data()
    // };

    vk::DescriptorSetLayoutCreateInfo layout_info{
        // .pNext        = &flags_info,
        .flags        = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
        .bindingCount = static_cast<uint32_t>(layout_bindings.size()),
        .pBindings    = layout_bindings.data()
    };

    _descriptor_set_layout = vk::raii::DescriptorSetLayout(_device, layout_info);
  }

  void create_graphics_pipeline() {
    // vk::raii::ShaderModule shader_module = create_shader_module(
    //     _device, read_file(vulkan_config::shader_path)
    // );

    vk::ShaderModuleCreateInfo shader_module_info{
        .codeSize = get_app_context()->shader_modules[0].size(),
        .pCode    = get_app_context()->shader_modules[0].data()
    };
    vk::raii::ShaderModule shader_module(_device, shader_module_info);

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages{
        {.stage = vk::ShaderStageFlagBits::eVertex, .module = shader_module, .pName = "vertMain"},
        {.stage = vk::ShaderStageFlagBits::eFragment, .module = shader_module, .pName = "fragMain"}
    };

    auto bindingDescription   = vertex_3D::binding_description();
    auto attributeDescription = vertex_3D::attribute_description();
    vk::PipelineVertexInputStateCreateInfo vertex_input_info{
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size()),
        .pVertexAttributeDescriptions    = attributeDescription.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{
        .topology = vk::PrimitiveTopology::eTriangleList,
    };

    std::vector<vk::DynamicState> dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        // vk::DynamicState::ePrimitiveTopology,
        // vk::DynamicState::ePrimitiveRestartEnable
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_info{
        .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
        .pDynamicStates    = dynamic_states.data()
    };

    vk::PipelineViewportStateCreateInfo viewport_state_info{.viewportCount = 1, .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eNone,
        .frontFace               = vk::FrontFace::eClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0F
    };
    // Line width greater than 1.0 requires GPU extension

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState color_blend_attachment{
        .blendEnable         = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                               | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineDepthStencilStateCreateInfo depth_stencil{
        .depthTestEnable       = vk::True,
        .depthWriteEnable      = vk::True,
        .depthCompareOp        = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False
    };
    vk::PipelineColorBlendStateCreateInfo color_blending{
        .logicOpEnable   = vk::False,
        .logicOp         = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment
    };

    vk::PushConstantRange push_constants_range{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset     = 0,
        .size       = sizeof(PushConstants)
    };

    vk::PipelineLayoutCreateInfo pipeline_layout_info{
        .setLayoutCount         = 1,
        .pSetLayouts            = &*_descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &push_constants_range,
    };

    _pipeline_layout = vk::raii::PipelineLayout(_device, pipeline_layout_info);

    vk::GraphicsPipelineCreateInfo graphics_pipeline_info{
        .stageCount          = static_cast<uint32_t>(shader_stages.size()),
        .pStages             = shader_stages.data(),
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState      = &viewport_state_info,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depth_stencil,
        .pColorBlendState    = &color_blending,
        .pDynamicState       = &dynamic_state_info,
        .layout              = _pipeline_layout,
        .renderPass          = nullptr,
    };

    vk::PipelineRenderingCreateInfo pipeline_rendering_info{
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &_swapchain_format.format,
        .depthAttachmentFormat   = vk::Format::eD32Sfloat
    };

    vk::StructureChain<
        vk::GraphicsPipelineCreateInfo,
        vk::PipelineRenderingCreateInfo>
        pipeline_info_chain = {
            graphics_pipeline_info,  // NULL because we use dynamic rendering
            pipeline_rendering_info
        };

    _graphics_pipeline = vk::raii::Pipeline(
        _device, nullptr, pipeline_info_chain.get<vk::GraphicsPipelineCreateInfo>()
    );
  }

  void recreate_graphics_pipeline() {
    _device.waitIdle();
    _graphics_pipeline = nullptr;
    create_graphics_pipeline();
  }

  void create_command_pool() {
    vk::CommandPoolCreateInfo pool_info{
        .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = _graphics_queue_family
    };
    _command_pool = vk::raii::CommandPool(_device, pool_info);
  }

  void create_depth_resources() {
    _depth_image = create_image(
        _allocator,
        _device,
        vk::Format::eD32Sfloat,
        _swapchain_extent.width,
        _swapchain_extent.height,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth
    );
    auto cmd = begin_single_command_buffer(_command_pool, _device);
    transition_image_layout(
        *_depth_image.image,
        cmd,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests
            | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests
            | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth
    );
    submit_single_command_buffer(_graphics_queue, std::move(cmd));
  }

  void create_texture_sampler() {
    auto properties = _physical_device.getProperties();
    vk::SamplerCreateInfo sampler_info{
        .magFilter               = vk::Filter::eNearest,
        .minFilter               = vk::Filter::eNearest,
        .mipmapMode              = vk::SamplerMipmapMode::eNearest,
        .addressModeU            = vk::SamplerAddressMode::eRepeat,
        .addressModeV            = vk::SamplerAddressMode::eRepeat,
        .addressModeW            = vk::SamplerAddressMode::eRepeat,
        .mipLodBias              = 0.0F,
        .anisotropyEnable        = vk::True,
        .maxAnisotropy           = properties.limits.maxSamplerAnisotropy,
        .compareEnable           = vk::False,
        .compareOp               = vk::CompareOp::eAlways,
        .minLod                  = 0.0F,
        .maxLod                  = 0.0F,
        .borderColor             = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False
    };

    _image_sampler = vk::raii::Sampler(_device, sampler_info);
  }

  void create_descriptor_pools() {
    std::vector<vk::DescriptorPoolSize> pool_sizes{vk::DescriptorPoolSize{
        .type            = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = vulkan_config::frames_in_flight * vulkan_config::max_number_of_textures
    }};
    vk::DescriptorPoolCreateInfo pool_info{
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
                         | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        .maxSets       = vulkan_config::frames_in_flight,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes    = pool_sizes.data()
    };

    _descriptor_pool = vk::raii::DescriptorPool(_device, pool_info);
  }

  void create_descriptor_sets() {
    std::vector<vk::DescriptorSetLayout> layouts(
        vulkan_config::frames_in_flight, *_descriptor_set_layout
    );
    vk::DescriptorSetAllocateInfo descriptor_set_info{
        .descriptorPool     = _descriptor_pool,
        .descriptorSetCount = vulkan_config::frames_in_flight,
        .pSetLayouts        = layouts.data()
    };

    _descriptor_sets = _device.allocateDescriptorSets(descriptor_set_info);

    // stb_image beer;
    // beer.load("resources/textures/ber.png");
    // vulkan_context ah = {
    //     .graphics_queue = &_graphics_queue,
    //     .device         = &_device,
    //     .allocator      = &_allocator,
    //     .command_pool   = &_command_pool,
    // };
    // image = load_texture_to_gpu(ah, std::move(beer));

    // for (uint32_t i = 0; i < vulkan_config::frames_in_flight; i++) {
    //   vk::DescriptorImageInfo imageInfo{
    //       .sampler     = _image_sampler,
    //       .imageView   = image.view,
    //       .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    //   };

    //   std::vector<vk::WriteDescriptorSet> write_descriptor_set{vk::WriteDescriptorSet{
    //       .dstSet          = _descriptor_sets.at(i),
    //       .dstBinding      = 0,
    //       .dstArrayElement = 0,
    //       .descriptorCount = 1,
    //       .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
    //       .pImageInfo      = &imageInfo
    //   }};

    //   _device.updateDescriptorSets(write_descriptor_set, {});
    // }
  }

  void create_command_buffer() {
    vk::CommandBufferAllocateInfo command_buffer_info{
        .commandPool        = _command_pool,
        .level              = vk::CommandBufferLevel::ePrimary,  // Secondary is called from primary
        .commandBufferCount = vulkan_config::frames_in_flight
    };

    _command_buffers = vk::raii::CommandBuffers(_device, command_buffer_info);
  }

  void record_command_buffer(uint32_t image_index) {
    auto& command_buffer = _command_buffers.at(_frame_index);
    vk::CommandBufferBeginInfo beginInfo{};
    command_buffer.begin(beginInfo);

    transition_image_layout(
        _swapchain_images.at(image_index),
        command_buffer,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor
    );

    vk::RenderingAttachmentInfo color_attachment_info{
        .imageView   = _swapchain_views.at(image_index),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = vulkan_config::clear_color
    };

    vk::RenderingAttachmentInfo depth_attachment_info{
        .imageView   = _depth_image.view,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eDontCare,
        .clearValue  = vulkan_config::clear_depth
    };

    vk::RenderingInfo renderingInfo{
        .renderArea           = {.offset = {.x = 0, .y = 0}, .extent = _swapchain_extent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment_info,
        .pDepthAttachment     = &depth_attachment_info,
    };

    command_buffer.beginRendering(renderingInfo);

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphics_pipeline);

    command_buffer.setViewport(
        0,
        vk::Viewport{
            .x        = 0.0F,
            .y        = static_cast<float>(_swapchain_extent.height),
            .width    = static_cast<float>(_swapchain_extent.width),
            .height   = -static_cast<float>(_swapchain_extent.height),
            .minDepth = 0.0F,
            .maxDepth = 1.0F
        }
    );
    command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapchain_extent));

    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        _pipeline_layout,
        0,
        *_descriptor_sets.at(_frame_index),
        nullptr
    );

    command_buffer.pushConstants<PushConstants>(
        _pipeline_layout,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0,
        _push_constants
    );

    for (auto& mesh : *get_app_context()->resources.meshes) {
      mesh.render(command_buffer);
    }

    command_buffer.endRendering();

    transition_image_layout(
        _swapchain_images.at(image_index),
        command_buffer,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::ImageAspectFlagBits::eColor
    );

    command_buffer.end();
  }

  void create_synchronisation_objects() {
    vk::SemaphoreCreateInfo semaphore_info{};
    // Initialise with 'signaled' otherwise wait indefinitely for non existent "frame 0"
    vk::FenceCreateInfo fence_info{.flags = vk::FenceCreateFlagBits::eSignaled};

    for (uint32_t i = 0; i < vulkan_config::frames_in_flight; i++) {
      _presentation_semaphores.emplace_back(_device, semaphore_info);
      _draw_fences.emplace_back(_device, fence_info);
    }
  }

  void draw_frame() {
    auto fence_result = _device.waitForFences(
        *_draw_fences.at(_frame_index), vk::True, std::numeric_limits<uint64_t>::max()
    );
    if (fence_result != vk::Result::eSuccess) {
      throw std::runtime_error("Waiting for fence failed: " + vk::to_string(fence_result));
    }

    auto [result, image_index] = _swapchain.acquireNextImage(
        std::numeric_limits<uint64_t>::max(),
        *_presentation_semaphores.at(_frame_index),
        nullptr
    );  // Timeout, semaphore, fence

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
      vk::SemaphoreCreateInfo semaphore_info{};
      _presentation_semaphores.at(_frame_index) = vk::raii::Semaphore(_device, semaphore_info);
      recreate_swapchain();
      return;
    } else if (result != vk::Result::eSuccess) {
      throw std::runtime_error("Swap chain image acquisition failed: " + vk::to_string(result));
    }

    _device.resetFences(*_draw_fences.at(_frame_index));

    _command_buffers.at(_frame_index).reset();

    record_command_buffer(image_index);

    vk::PipelineStageFlags wait_destination_stage(
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    );

    vk::SubmitInfo submit_info{
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &*_presentation_semaphores.at(_frame_index),
        .pWaitDstStageMask    = &wait_destination_stage,  // Semaphore to wait for + pipeline stage
        .commandBufferCount   = 1,
        .pCommandBuffers      = &*_command_buffers.at(_frame_index),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &*_swapchain_semaphores.at(image_index)
    };  // Semaphore to signal when done

    _graphics_queue.submit(submit_info, *_draw_fences.at(_frame_index));

    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*_swapchain_semaphores.at(image_index),
        .swapchainCount     = 1,
        .pSwapchains        = &*_swapchain,
        .pImageIndices      = &image_index
    };

    result = _graphics_queue.presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
      recreate_swapchain();
      return;
    } else if (result != vk::Result::eSuccess) {
      throw std::runtime_error("Graphics queue presentation failed: " + vk::to_string(result));
    }

    _frame_index = (_frame_index + 1) % vulkan_config::frames_in_flight;
  }

  static inline bool _instance_running           = false;
  static inline vega_console _console            = console::create("Vulkan");
  static inline vega_console _validation_console = console::create("VulkanValidation");

  bool _initialised = false;
  vk::raii::Context _context;
  vk::raii::Instance _instance                      = nullptr;
  vk::raii::DebugUtilsMessengerEXT _debug_messenger = nullptr;
  vk::raii::SurfaceKHR _surface                     = nullptr;
  vk::raii::PhysicalDevice _physical_device         = nullptr;

  uint32_t _graphics_queue_family = ~0;

  vk::raii::Queue _graphics_queue   = nullptr;
  vk::raii::Device _device          = nullptr;
  vma::raii::Allocator _allocator   = nullptr;
  vk::raii::SwapchainKHR _swapchain = nullptr;
  vk::SurfaceFormatKHR _swapchain_format{};
  vk::Extent2D _swapchain_extent{};
  std::vector<vk::Image> _swapchain_images;
  std::vector<vk::raii::ImageView> _swapchain_views;
  std::vector<vk::raii::Semaphore> _swapchain_semaphores;

  vk::raii::Pipeline _graphics_pipeline = nullptr;
  PushConstants _push_constants;
  vk::raii::PipelineLayout _pipeline_layout = nullptr;

  vk::raii::CommandPool _command_pool = nullptr;

  gpu_image _depth_image;

  vk::raii::Sampler _image_sampler = nullptr;

  vk::raii::DescriptorSetLayout _descriptor_set_layout = nullptr;
  vk::raii::DescriptorPool _descriptor_pool            = nullptr;
  vk::raii::DescriptorSets _descriptor_sets            = nullptr;

  std::vector<vk::raii::CommandBuffer> _command_buffers;
  uint32_t _frame_index = 0;
  std::vector<vk::raii::Semaphore> _presentation_semaphores;
  std::vector<vk::raii::Fence> _draw_fences;
};
