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
      auto beer_tex = scene.load_texture(vk_context, textures::beer);
      auto beer_mat = resources.materials.push(beer_tex);

      auto viking_tex = scene.load_texture(vk_context, textures::viking);
      auto viking_mat = resources.materials.push(viking_tex);

      // MODELS
      // scene.load_mesh_from_obj_mtl(vk_context, meshes::viking, viking_mat);
      // scene.load_mesh_from_obj_mtl(vk_context, meshes::tyra);
      scene.load_mesh_from_obj_mtl(vk_context, meshes::sponza);
      // scene.load_mesh_from_obj_mtl(vk_context, meshes::tank);
      scene.load_vertex_array(create_cube({1.F, 0.F, 1.F}, 0.5F), beer_mat);

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