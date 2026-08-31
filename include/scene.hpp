#pragma once

#include <cstdint>
#include <stdexcept>
#include "allocator.hpp"
#include "glm/gtc/random.hpp"
#include "gpu_objects.hpp"
#include "mesh.hpp"
#include "resources/handle.hpp"
#include "vulkan/context.hpp"
#include "material.hpp"
#include "vulkan/config.hpp"
#include "resources/mesh_imports.hpp"
#include "object_loader.hpp"
#include "image.hpp"

struct scene_resources {
  static_allocator<mesh_3D> meshes;
  static_allocator<material> materials;
  static_allocator<gpu_image> textures;
  vk::raii::Sampler sampler         = nullptr;
  vma::raii::Buffer material_buffer = nullptr;
};

class scene {
 public:
  scene() = default;
  ~scene() { clear(); }

  [[nodiscard]] scene_resources* resources() { return &_resources; }
  [[nodiscard]] static_allocator<mesh_3D>* meshes() { return &_resources.meshes; }
  [[nodiscard]] static_allocator<material>* materials() { return &_resources.materials; }
  [[nodiscard]] static_allocator<gpu_image>* textures() { return &_resources.textures; }

  /** Should be followed by a call to 'update_texture_descriptor(app_context*)' */
  handle<mesh_3D> load_object_and_materials(vulkan_context vk_context, const model_info& info) {
    if (!info.mesh.valid) {
      auto [vertices, obj_materials] = load_object(
          info.object_folder + info.mesh_name, info.scale, info.z_is_up
      );

      if (!obj_materials.empty()) {
        std::vector<handle<material>> materials;
        for (auto& mat : obj_materials) {
          stb_image texture;
          if (!mat.diffuse_texname.empty()) {
            texture = stb_image(info.object_folder + mat.diffuse_texname);
          } else {
            auto color = glm::vec4(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1);
            console::get(consoles::assets)
                ->warn(
                    "No texture specified for material {}, replacing by 1x1 texture of color "
                    "({},{},{})",
                    mat.name,
                    color.r,
                    color.g,
                    color.b
                );
            texture = stb_image(color, 1, 1);
          }

          auto texture_handle = _resources.textures.push(
              std::move(load_texture_to_gpu(vk_context, std::move(texture)))
          );

          auto material_handle = _resources.materials.push(texture_handle);
          materials.push_back(material_handle);
        }
        for (auto& vertex : vertices) {
          vertex.material_id = _resources.materials.get_index(materials[vertex.material_id]);
        }
      } else {
        for (auto& vertex : vertices) {
          vertex.material_id = 0;
        }
      }
      info.mesh = _resources.meshes.push(vertices);
    }
    return info.mesh;
  }

  handle<mesh_3D>
  load_vertex_array(std::vector<vertex_3D> vertices, handle<material> material = {}) {
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
      uint32_t texture_index = _resources.textures.get_index(mat.get_albedo());
      g_materials.emplace_back(glm::ballRand(1.F), texture_index);
    }

    _resources.material_buffer.getAllocation().copyFromMemory(g_materials.data(), 0, size);
  }

  void update_texture_descriptor(vulkan_context vk_context) {
    console::get(consoles::assets)
        ->warn(
            "TO DO: move to a fixed size arena for textures and make it efficient to update "
            "descriptors"
        );
    std::vector<vk::DescriptorImageInfo> image_descriptors;
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
  scene_resources _resources;
};