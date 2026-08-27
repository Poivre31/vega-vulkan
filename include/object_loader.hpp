#pragma once
#include <console/console.hpp>
#include <filesystem>
#include "stb_image.hpp"
#include "tiny_obj_loader.h"
#include "mesh.hpp"

// class material_manager {
//  public:
//   static uint32_t generate_material_id() { return _current_id++; }
//   static std::unordered_map<int, uint32_t> add_materials() {}

//  private:
//   static inline uint32_t _current_id = 0;
//   // static inline std::unordered_map<u, class Ty>
// };

std::pair<std::vector<vertex_3D>, std::vector<tinyobj::material_t>>
load_object(const std::string& model_path, float scale) {  // NOLINT
  bool silence = false;
  try {
    auto path = std::filesystem::path(model_path);
    if (!std::filesystem::exists(path)) {
      path = std::filesystem::path("resources/models/").append(model_path);
      if (!std::filesystem::exists(path)) {
        console::get(consoles::assets)
            ->error(
                "Model '{}' doesn't exist in directory '{}' or '{}', error",
                model_path,
                std::filesystem::current_path().string(),
                std::filesystem::absolute("resources/models/").string()
            );
        std::abort();
      }
    }
    if (!silence) {
      console::get(consoles::assets)->trace("Loading model: found model at '{:s}'", path.string());
    }

    tinyobj::ObjReaderConfig reader_config{};
    tinyobj::ObjReader reader{};

    if (!reader.ParseFromFile(model_path, reader_config)) {
      if (!reader.Error().empty()) {
        console::get(consoles::assets)
            ->error("TinyObjReader error when loading model, aborting: {:s}", reader.Error());
      }
      std::abort();
    }
    if (!reader.Warning().empty() && vulkan_config::enable_validation) {  // TO DO: Separate config
      console::get(consoles::assets)
          ->warn("TinyObjReader warning when loading model: {:s}", reader.Warning());
    }

    const auto& attrib    = reader.GetAttrib();
    const auto& shapes    = reader.GetShapes();
    const auto& materials = reader.GetMaterials();

    std::vector<vertex_3D> vertices;

    for (const auto& shape : shapes) {
      size_t shape_offset = 0;
      for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); face++) {
        auto face_vertices = static_cast<size_t>(shape.mesh.num_face_vertices[face]);
        if (face_vertices != 3) {
          console::get(consoles::assets)->error("Non triangle face when loading model object");
          continue;
        }
        for (size_t v = 0; v < face_vertices; v++) {
          tinyobj::index_t idx = shape.mesh.indices[shape_offset + v];
          vertex_3D vertex;
          vertex.position.x = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0]
                              * scale;
          vertex.position.z = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1]
                              * scale;
          vertex.position.y = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2]
                              * scale;

          if (idx.normal_index >= 0) {
            vertex.normal.x = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
            vertex.normal.z = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
            vertex.normal.y = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];
          }

          if (idx.texcoord_index >= 0) {
            vertex.uv.x = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
            vertex.uv.y = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
          }

          vertex.material_id = shape.mesh.material_ids[face];

          vertices.push_back(vertex);
        }
        shape_offset += face_vertices;

        // shapes[s].mesh.material_ids[f];
      }
    }

    return {vertices, materials};

  } catch (const std::filesystem::filesystem_error& e) {
    console::get(consoles::assets)
        ->error("Filesystem error loading model '{}': {}", model_path, e.what());
  } catch (const std::exception& e) {
    console::get(consoles::assets)
        ->error("Unexpected error loading model'{}': {}", model_path, e.what());
  }
  std::abort();
}

std::pair<std::vector<vertex_3D>, std::vector<stb_image>> load_object_and_materials(
    const std::string& model_directory,
    const std::string& model_name,
    float scale
) {
  auto [vertices, materials] = load_object(model_directory + model_name, scale);

  std::vector<stb_image> images;
  images.reserve(materials.size());
  for (auto& material : materials) {
    if (!material.diffuse_texname.empty()) {
      images.emplace_back(model_directory + material.diffuse_texname);
    } else {
      auto color = glm::vec4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1);
      console::get(consoles::assets)
          ->warn(
              "No texture specified for material {}, replacing by 1x1 texture of color ({},{},{})",
              material.name,
              color.r,
              color.g,
              color.b
          );
      images.emplace_back(color, 1, 1);
    }
  }

  return {vertices, std::move(images)};
}