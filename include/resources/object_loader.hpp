#pragma once
#include <console/console.hpp>
#include <filesystem>
#include <utility>
#include <timer/timer.hpp>

#include "tiny_obj_loader.h"
#include "mesh.hpp"

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

    tinyobj::basic_attrib_t<> attrib;
    std::vector<tinyobj::basic_shape_t<>> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    tinyobj::OptLoadConfig config;
    config.triangulate = true;
    config.num_threads = -1;

    bool ok = tinyobj::LoadObjOpt(
        &attrib,
        &shapes,
        &materials,
        &warn,
        &err,
        model_path.data(),
        /* mtl_basedir */ nullptr,
        config
    );
    if (!warn.empty()) {
      console::get(consoles::assets)->warn("Tinyobj loader warning : {}", warn);
    }
    if (!ok) {
      console::get(consoles::assets)->warn("Tinyobj loader error : {}", err);
      std::abort();
    }

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
