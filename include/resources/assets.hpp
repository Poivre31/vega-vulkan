#pragma once

#include <vector>

#include "graphics/context.hpp"
#include "graphics/image.hpp"

#include "application/scene.hpp"
#include "application/layer.hpp"

#include "mesh_imports.hpp"
#include "primitive_meshes.hpp"  // IWYU pragma: keep
#include "texture_imports.hpp"
#include "stb_image.hpp"

void update_texture_descriptor(vulkan_context vk_context, scene_resources& resources) {
  console::get(consoles::assets)
      ->warn(
          "TO DO: move to a fixed size arena for textures and make it efficient to update "
          "descriptors"
      );
  std::vector<vk::DescriptorImageInfo> image_descriptors;
  for (auto& texture : resources.textures) {
    vk::DescriptorImageInfo image_info{
        .sampler     = resources.sampler,
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

class assets_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  bool init() noexcept final {
    try {
      vulkan_context vk_context = get_app_context()->vulkan;
      auto& scene               = get_app_context()->active_scene;
      auto& resources           = *scene.resources();

      scene.init(vk_context);

      // TEXTURES
      auto beer_tex = resources.textures.push(
          std::move(load_texture_to_gpu(vk_context, stb_image(textures::beer.texture_path)))
      );
      auto beer_mat = resources.materials.push(beer_tex);

      auto viking_tex = resources.textures.push(
          std::move(load_texture_to_gpu(vk_context, stb_image(textures::viking.texture_path)))
      );
      auto viking_mat = resources.materials.push(viking_tex);

      // MODELS
      // scene.load_mesh_from_obj_mtl(vk_context, meshes::viking, viking_mat);
      // scene.load_mesh_from_obj_mtl(vk_context, meshes::tyra);
      scene.load_mesh_from_obj_mtl(vk_context, meshes::sponza);
      // scene.load_mesh_from_obj_mtl(vk_context, meshes::tank);
      // scene.load_vertex_array(create_cube({1.F, 0.F, 1.F}, 0.5F), beer_mat);

      for (auto& mesh : resources.meshes) {
        mesh.create_vertex_buffer(vk_context);
      }

      // std::vector<stb_image> cpu_images;
      // cpu_images.emplace_back(textures::beer.texture_path);
      // for (auto& image : cpu_images) {
      //   resources.textures.push(std::move(load_texture_to_gpu(vk_context, std::move(image))));
      // }

      scene.update_texture_descriptor(vk_context);
      scene.upload_material_buffer(vk_context);

    } catch (const std::exception& e) {
      console::get(consoles::assets)->error("Exception during assets initialisation: {}", e.what());
      return false;
    } catch (...) {
      console::get(consoles::assets)->error("Unknown error during assets initialisation {}");
      return false;
    }

    return true;
  }
  void update(double dt) noexcept final {}
  void cleanup() noexcept final {}

 private:
};