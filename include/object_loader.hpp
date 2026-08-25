#pragma once
#include <console/console.hpp>
#include <filesystem>
#include "tiny_obj_loader.h"
#include "mesh.hpp"

std::vector<vertex_3D> load_object(const std::string& model_path) {  // NOLINT
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
    if (!reader.Warning().empty()) {
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
        }
        for (size_t v = 0; v < face_vertices; v++) {
          tinyobj::index_t idx = shape.mesh.indices[shape_offset + v];
          vertex_3D vertex;
          vertex.position.x = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0];
          vertex.position.z = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1];
          vertex.position.y = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2];

          if (idx.normal_index >= 0) {
            vertex.normal.x = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
            vertex.normal.z = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
            vertex.normal.y = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];
          }

          if (idx.texcoord_index >= 0) {
            vertex.uv.x = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
            vertex.uv.y = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];
          }
          vertices.push_back(vertex);
        }
        shape_offset += face_vertices;
      }
    }

    return vertices;

  } catch (const std::filesystem::filesystem_error& e) {
    console::get(consoles::assets)
        ->error("Filesystem error loading model '{}': {}", model_path, e.what());
  } catch (const std::exception& e) {
    console::get(consoles::assets)
        ->error("Unexpected error loading model'{}': {}", model_path, e.what());
  }
  std::abort();
}
