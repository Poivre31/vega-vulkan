#pragma once

#include <console/console.hpp>

#include "context.hpp"
#include "image.hpp"

enum class shader_storage_type : uint8_t { ro_image, rw_image, ro_buffer, rw_buffer };

struct PC_post_processing_data {
  float time;
  uint32_t frame;

  float dithering_stength;

  int blur_kernel_radius;
  float blur_strength;

  float exposure;
  uint32_t srgb_transform;
  float srgb_gamma;
  float srgb_offset;

  float znear;
  //   float zfar;

  uint32_t enable_fog;
  glm::vec3 fog_color;
  float fog_falloff;
};

template <typename push_constants>
class compute_shader {
 public:
  compute_shader(nullptr_t) {}
  compute_shader(
      vk::raii::Device& device,
      const std::vector<char>& shader_code,
      const std::vector<shader_storage_type>& storages
  ) {
    // COMPUTE PIPELINE CREATION
    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
    uint32_t binding = 0;
    for (auto storage : storages) {
      vk::DescriptorType descriptor_type{};
      switch (storage) {
        case shader_storage_type::ro_image: {
          descriptor_type = vk::DescriptorType::eSampledImage;
          number_of_sampled_images++;
        } break;
        case shader_storage_type::rw_image: {
          descriptor_type = vk::DescriptorType::eStorageImage;
          number_of_storage_images++;
        } break;
        case shader_storage_type::ro_buffer: {
          throw std::runtime_error("RO buffer not implemented");
        } break;
        case shader_storage_type::rw_buffer: {
          descriptor_type = vk::DescriptorType::eStorageBuffer;
          number_of_buffers++;
        } break;
      }
      layout_bindings.push_back(
          {.binding         = binding,
           .descriptorType  = descriptor_type,
           .descriptorCount = 1,
           .stageFlags      = vk::ShaderStageFlagBits::eCompute}
      );
      binding++;
    }

    vk::DescriptorSetLayoutCreateInfo layout_info{
        .bindingCount = static_cast<uint32_t>(layout_bindings.size()),
        .pBindings    = layout_bindings.data()
    };

    _descriptor_set_layout = vk::raii::DescriptorSetLayout(device, layout_info);
    if (!*_descriptor_set_layout) {
      throw std::runtime_error("Descriptor set layout creation silently failed");
    }

    vk::ShaderModuleCreateInfo shader_module_info{
        .codeSize = shader_code.size(),
        .pCode    = reinterpret_cast<const uint32_t*>(shader_code.data())
    };
    _shader_module = vk::raii::ShaderModule(device, shader_module_info);
    if (!*_shader_module) {
      throw std::runtime_error("Shader module creation silently failed");
    }
    vk::PipelineShaderStageCreateInfo shader_stage_info{
        .stage = vk::ShaderStageFlagBits::eCompute, .module = _shader_module, .pName = "main"
    };
    vk::PushConstantRange range{
        .stageFlags = vk::ShaderStageFlagBits::eCompute, .offset = 0, .size = sizeof(push_constants)
    };

    vk::PipelineLayoutCreateInfo pipeline_layout_info{
        .setLayoutCount         = 1,
        .pSetLayouts            = &*_descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &range
    };

    _pipeline_layout = vk::raii::PipelineLayout(device, pipeline_layout_info);
    if (!*_pipeline_layout) {
      throw std::runtime_error("Compute pipeline layout creation silently failed");
    }

    vk::ComputePipelineCreateInfo pipeline_info{
        .stage = shader_stage_info, .layout = _pipeline_layout
    };
    _compute_pipeline = device.createComputePipeline(nullptr, pipeline_info);
    if (!*_compute_pipeline) {
      throw std::runtime_error("Compute pipeline creation failed silently");
    } else {
      console::get(consoles::graphics)->trace("Created vulkan compute pipeline");
    }

    // DESCRIPTOR SET UP
    std::vector<vk::DescriptorPoolSize> pool_sizes;
    if (number_of_sampled_images > 0) {
      pool_sizes.emplace_back(
          vk::DescriptorPoolSize{
              .type = vk::DescriptorType::eSampledImage, .descriptorCount = number_of_sampled_images
          }
      );
    }
    if (number_of_storage_images > 0) {
      pool_sizes.emplace_back(
          vk::DescriptorPoolSize{
              .type = vk::DescriptorType::eStorageImage, .descriptorCount = number_of_storage_images
          }
      );
    }
    if (number_of_buffers > 0) {
      pool_sizes.emplace_back(
          vk::DescriptorPoolSize{
              .type = vk::DescriptorType::eStorageBuffer, .descriptorCount = number_of_buffers
          }
      );
    }

    vk::DescriptorPoolCreateInfo pool_info = {
        .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets       = 1,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes    = pool_sizes.data()
    };

    _descriptor_pool = vk::raii::DescriptorPool(device, pool_info);
    if (!*_descriptor_pool) {
      throw std::runtime_error("Descriptor pool creation silently failed");
    }

    vk::DescriptorSetLayout layout                    = *_descriptor_set_layout;
    vk::DescriptorSetAllocateInfo descriptor_set_info = {
        .descriptorPool = _descriptor_pool, .descriptorSetCount = 1, .pSetLayouts = &layout
    };

    _descriptor_set = std::move(device.allocateDescriptorSets(descriptor_set_info).front());
  }

  void update_descriptor_sets(
      vk::raii::Device& device,
      const std::vector<gpu_image*>& sample_images,
      const std::vector<gpu_image*>& storage_images,
      const std::vector<vma::raii::Buffer*>& buffers
  ) {
    std::vector<vk::WriteDescriptorSet> write_descriptor_sets;
    std::vector<vk::DescriptorImageInfo> sample_image_descriptors(sample_images.size());
    std::vector<vk::DescriptorImageInfo> storage_image_descriptors(storage_images.size());
    std::vector<vk::DescriptorBufferInfo> buffer_descriptors(buffers.size());
    if (sample_images.size() != number_of_sampled_images) {
      throw std::runtime_error(
          fmt::format(
              "Compute shader was created with {} sample images but {} were passed during "
              "descriptor set update",
              number_of_sampled_images,
              sample_images.size()
          )
      );
    }
    if (storage_images.size() != number_of_storage_images) {
      throw std::runtime_error(
          fmt::format(
              "Compute shader was created with {} storage images but {} were passed during "
              "descriptor set update",
              number_of_storage_images,
              storage_images.size()
          )
      );
    }
    if (buffers.size() != number_of_buffers) {
      throw std::runtime_error(
          fmt::format(
              "Compute shader was created with {} storage buffers but {} were passed during "
              "descriptor set update",
              number_of_buffers,
              buffers.size()
          )
      );
    }

    uint32_t binding = 0;
    for (const auto* image : sample_images) {
      if (!image) {
        throw std::runtime_error(
            "Passed a nullptr to compute shader sample image descriptor set update"
        );
      }
      sample_image_descriptors[binding] = vk::DescriptorImageInfo{
          .sampler = nullptr, .imageView = image->view, .imageLayout = vk::ImageLayout::eGeneral
      };
      write_descriptor_sets.emplace_back(
          vk::WriteDescriptorSet{
              .dstSet          = _descriptor_set,
              .dstBinding      = binding,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType  = vk::DescriptorType::eSampledImage,
              .pImageInfo      = &sample_image_descriptors[binding]
          }
      );
      binding++;
    }
    binding = 0;
    for (const auto* image : storage_images) {
      if (!image) {
        throw std::runtime_error(
            "Passed a nullptr to compute shader storage image descriptor set update"
        );
      }
      storage_image_descriptors[binding] = vk::DescriptorImageInfo{
          .sampler = nullptr, .imageView = image->view, .imageLayout = vk::ImageLayout::eGeneral
      };
      write_descriptor_sets.emplace_back(
          vk::WriteDescriptorSet{
              .dstSet          = _descriptor_set,
              .dstBinding      = binding + number_of_sampled_images,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType  = vk::DescriptorType::eStorageImage,
              .pImageInfo      = &storage_image_descriptors[binding]
          }
      );
      binding++;
    }
    binding = 0;
    for (const auto* buffer : buffers) {
      if (!buffer) {
        throw std::runtime_error(
            "Passed a nullptr to compute shader storage buffer descriptor set update"
        );
      }
      buffer_descriptors[binding] = vk::DescriptorBufferInfo{
          .buffer = *buffer, .offset = 0, .range = 1
      };
      write_descriptor_sets.emplace_back(
          vk::WriteDescriptorSet{
              .dstSet          = _descriptor_set,
              .dstBinding      = binding + number_of_sampled_images + number_of_storage_images,
              .dstArrayElement = 0,
              .descriptorCount = 1,
              .descriptorType  = vk::DescriptorType::eStorageBuffer,
              .pBufferInfo     = &buffer_descriptors[binding]
          }
      );
      binding++;
    }

    device.updateDescriptorSets(write_descriptor_sets, {});
  }

  void bind(vk::raii::CommandBuffer& cmd) {
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, _compute_pipeline);
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute, _pipeline_layout, 0, {*_descriptor_set}, {}
    );
  }

  void dispatch(
      vk::raii::CommandBuffer& cmd,
      push_constants constants,
      uint32_t workgroup_width,
      uint32_t workgroup_height,
      uint32_t width,
      uint32_t height
  ) {
    cmd.pushConstants<push_constants>(
        _pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, {constants}
    );

    cmd.dispatch(
        (width + int(workgroup_width) - 1) / workgroup_width,
        (height + int(workgroup_height) - 1) / workgroup_height,
        1
    );
  }

 private:
  vk::raii::DescriptorSetLayout _descriptor_set_layout = nullptr;
  vk::raii::ShaderModule _shader_module                = nullptr;
  vk::raii::PipelineLayout _pipeline_layout            = nullptr;
  vk::raii::Pipeline _compute_pipeline                 = nullptr;
  vk::raii::DescriptorPool _descriptor_pool            = nullptr;
  vk::raii::DescriptorSet _descriptor_set              = nullptr;

  uint32_t number_of_sampled_images = 0;
  uint32_t number_of_storage_images = 0;
  uint32_t number_of_buffers        = 0;
};