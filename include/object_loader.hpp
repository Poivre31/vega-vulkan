#pragma once
#include <console/console.hpp>
#include <filesystem>
#include <utility>
#include "glm/geometric.hpp"
#include "layer.hpp"
#include "stb_image.hpp"
#include "timer/timer.hpp"
#include "tiny_obj_loader.h"
#include "mesh.hpp"
#include "resources/mesh_imports.hpp"
#include "resources/texture_imports.hpp"

// class material_manager {
//  public:
//   static uint32_t generate_material_id() { return _current_id++; }
//   static std::unordered_map<int, uint32_t> add_materials() {}

//  private:
//   static inline uint32_t _current_id = 0;
//   // static inline std::unordered_map<u, class Ty>
// };

std::pair<std::vector<vertex_3D>, std::vector<tinyobj::material_t>>
load_object(const std::string& model_path, float scale = 1.F, bool z_is_up = true) {  // NOLINT
  auto timer   = scoped_timer("object-loading");
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

    int x_offset = 0, y_offset = 1, z_offset = 2;
    if (!z_is_up) {
      std::swap(y_offset, z_offset);
    }

    for (const auto& shape : shapes) {
      size_t shape_offset = 0;
      for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); face++) {
        auto number_face_vertices = static_cast<size_t>(shape.mesh.num_face_vertices[face]);
        assert(number_face_vertices == 3 && "Non triangle face when loading model object");
        bool create_face_normals = false;
        std::array<vertex_3D, 3> face_vertices{};
        for (size_t v = 0; v < number_face_vertices; v++) {
          tinyobj::index_t idx = shape.mesh.indices[shape_offset + v];
          face_vertices.at(v).position.x =
              attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + x_offset] * scale;
          face_vertices.at(v).position.y =
              attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + y_offset] * scale;
          face_vertices.at(v).position.z =
              attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + z_offset] * scale;

          if (idx.normal_index >= 0) {
            face_vertices.at(v).normal.x =
                attrib.normals[3 * static_cast<size_t>(idx.normal_index) + x_offset];
            face_vertices.at(v).normal.y =
                attrib.normals[3 * static_cast<size_t>(idx.normal_index) + y_offset];
            face_vertices.at(v).normal.z =
                attrib.normals[3 * static_cast<size_t>(idx.normal_index) + z_offset];
          } else {
            create_face_normals = true;
          }

          if (idx.texcoord_index >= 0) {
            face_vertices.at(v).uv.x =
                attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
            face_vertices.at(v).uv.y =
                attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
          }

          face_vertices.at(v).material_id = shape.mesh.material_ids[face];
        }
        shape_offset += number_face_vertices;
        if (create_face_normals) {
          auto vec1   = face_vertices[1].position - face_vertices[0].position;
          auto vec2   = face_vertices[2].position - face_vertices[1].position;
          auto normal = -glm::cross(vec1, vec2);
          if (normal != glm::vec3(0.F)) {
            normal = glm::normalize(normal);
          }
          for (auto& vertex : face_vertices) {
            vertex.normal = normal;
          }
        }
        vertices.insert(vertices.end(), face_vertices.begin(), face_vertices.end());
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
    float scale  = 1.F,
    bool z_is_up = true
) {
  auto [vertices, materials] = load_object(model_directory + model_name, scale, z_is_up);

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

/** Should be followed by a call to 'update_texture_descriptor(app_context*)' */
handle<mesh_3D> load_object_and_materials(application_context* context, const model_info& info) {
  if (!info.mesh.valid) {
    auto [mesh_data, textures] = load_object_and_materials(
        info.object_folder, info.mesh_name, info.scale, info.z_is_up
    );
    auto texture_indices = upload_textures(context, std::move(textures));
    for (auto& vertex : mesh_data) {
      vertex.material_id = texture_indices[vertex.material_id].index;
    }

    mesh_3D mesh(mesh_data);
    mesh.create_vertex_buffer(context->vulkan);
    info.mesh = context->resources.meshes_.push(std::move(mesh));
  }
  return info.mesh;
}