#pragma once

#include "layer.hpp"
#include "mesh.hpp"
#include "primitive_meshes.hpp"

class assets_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  void init() noexcept final {
    try {
      _meshes.emplace_back(create_cube({1.F, 0.F, 0.F}, 0.3F));
      for (auto& mesh : _meshes) {
        mesh.create_vertex_buffer(get_app_context()->vulkan_context);
      }
      get_app_context()->resources.meshes = &_meshes;
    } catch (const std::exception& e) {
      console::get(consoles::assets)->error("Exception during assets initialisation: {}", e.what());
    } catch (...) {
      console::get(consoles::assets)->error("Unknown error during assets initialisation {}");
    }
  }
  void update(double dt) noexcept final {}
  void cleanup() noexcept final {}

 private:
  std::vector<mesh_3D> _meshes;
};