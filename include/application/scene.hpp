#pragma once

#include <cstdint>
#include <stdexcept>

#include "resources/allocator.hpp"
#include "resources/mesh.hpp"
#include "resources/handle.hpp"
#include "resources/material.hpp"
#include "resources/mesh_imports.hpp"
#include "resources/object_loader.hpp"

#include "graphics/image.hpp"
#include "graphics/context.hpp"
#include "graphics/config.hpp"
#include "graphics/gpu_objects.hpp"

struct scene_resources {
  static_allocator<mesh_3D> meshes;
  static_allocator<material> materials;
  static_allocator<gpu_image> textures;
  vk::raii::Sampler sampler         = nullptr;
  vma::raii::Buffer material_buffer = nullptr;

  handle<gpu_image> fallback_tex;
  handle<material> default_mat;
};

class scene {
 public:
  scene() = default;
  ~scene() { clear(); }

  scene(const scene&)            = delete;
  scene(scene&&)                 = delete;
  scene& operator=(const scene&) = delete;
  scene& operator=(scene&&)      = delete;

  void init(vulkan_context vk_context) {
    auto properties = vk_context.physical_device->getProperties();
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
    _resources.sampler = vk::raii::Sampler(*vk_context.device, sampler_info);

    auto fallback_tex = _resources.textures.push(
        std::move(load_texture_to_gpu(vk_context, stb_image()))
    );
    _resources.default_mat = _resources.materials.push(glm::vec4(1.F));

    _initialised = true;
  }

  [[nodiscard]] auto* resources() { return &_resources; }
  [[nodiscard]] auto* meshes() { return &_resources.meshes; }
  [[nodiscard]] auto* materials() { return &_resources.materials; }
  [[nodiscard]] auto* textures() { return &_resources.textures; }

  /** Should be followed by a call to 'update_texture_descriptor(app_context*)' */
  handle<mesh_3D> load_object_and_materials(vulkan_context vk_context, const model_info& info) {
    check_init();

    if (!info.mesh.valid) {
      auto [vertices, obj_materials] = load_object(
          info.object_folder + info.mesh_name, info.scale, info.z_is_up
      );

      if (!obj_materials.empty()) {
        std::vector<handle<material>> materials;
        for (auto& mat : obj_materials) {
          stb_image texture;
          handle<material> material_handle;
          auto color = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1);

          if (!mat.diffuse_texname.empty()) {
            texture = stb_image(info.object_folder + mat.diffuse_texname);

            auto texture_handle = _resources.textures.push(
                std::move(load_texture_to_gpu(vk_context, std::move(texture)))
            );

            material_handle = _resources.materials.push(color, texture_handle);
          } else {
            material_handle = _resources.materials.push(color);
          }

          materials.push_back(material_handle);
        }
        for (auto& vertex : vertices) {
          vertex.material_id = _resources.materials.get_index(materials[vertex.material_id]);
        }
      } else {
        for (auto& vertex : vertices) {
          vertex.material_id = _resources.materials.get_index(_resources.default_mat);
        }
      }
      info.mesh = _resources.meshes.push(vertices);
    }
    return info.mesh;
  }

  handle<mesh_3D>
  load_vertex_array(std::vector<vertex_3D> vertices, handle<material> material = {}) {
    check_init();

    uint32_t material_id = 0;
    if (material.valid) {
      material_id = _resources.materials.get_index(material);
    }
    for (auto& vertex : vertices) {
      vertex.material_id = material_id;
    }
    return _resources.meshes.push(vertices);
  }

  void upload_material_buffer(vulkan_context vk_context) {
    check_init();

    auto size = _resources.materials.size() * sizeof(gpu_material);
    if (size == 0) {
      throw std::runtime_error("No materials set");
    }
    vk::BufferCreateInfo buffer_info{
        .size        = size,
        .usage       = vk::BufferUsageFlagBits::eStorageBuffer
                       | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    vma::AllocationCreateInfo allocation_info{
        .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
        .usage = vma::MemoryUsage::eAutoPreferDevice
    };

    _resources.material_buffer = vk_context.allocator->createBuffer(buffer_info, allocation_info);

    std::vector<gpu_material> g_materials;
    g_materials.reserve(_resources.materials.size());
    for (auto& mat : _resources.materials) {
      auto tex   = mat.get_albedo_texture();
      auto color = mat.get_albedo_color();

      uint32_t texture_index = tex.valid ? _resources.textures.get_index(tex) : -1U;

      g_materials.emplace_back(color, texture_index);
    }

    _resources.material_buffer.getAllocation().copyFromMemory(g_materials.data(), 0, size);
  }

  void update_texture_descriptor(vulkan_context vk_context) {
    check_init();

    console::get(consoles::assets)
        ->warn(
            "TO DO: move to a fixed size arena for textures and make it efficient to update "
            "descriptors"
        );
    std::vector<vk::DescriptorImageInfo> image_descriptors;
    image_descriptors.reserve(_resources.textures.size());
    for (auto& texture : _resources.textures) {
      vk::DescriptorImageInfo image_info{
          .sampler     = _resources.sampler,
          .imageView   = texture.view,
          .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
      };
      image_descriptors.push_back(image_info);
    }

    for (uint32_t i = 0; i < vulkan_config::frames_in_flight; i++) {
      vk::WriteDescriptorSet write_descriptor_set{
          .dstSet          = vk_context.descriptor_sets->at(i),
          .dstBinding      = 0,
          .dstArrayElement = 0,
          .descriptorCount = static_cast<uint32_t>(image_descriptors.size()),
          .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
          .pImageInfo      = image_descriptors.data()
      };
      vk_context.device->updateDescriptorSets(write_descriptor_set, {});
    }
  }

  void clear() {
    _resources.meshes.clear();
    _resources.materials.clear();
    _resources.textures.clear();
    _resources.sampler         = nullptr;
    _resources.material_buffer = nullptr;
  }

 private:
  void check_init() const {
    if (!_initialised) {
      throw std::runtime_error(
          "Trying to modify a scene object that hasn't been initialised, call "
          "scene.init(vulkan_context) first"
      );
    }
  }
  bool _initialised = false;
  scene_resources _resources;
};