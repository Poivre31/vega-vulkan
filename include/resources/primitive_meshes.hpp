#pragma once

#include "mesh.hpp"

template <typename T>
constexpr std::vector<T> combine(const std::vector<T>& v1, const std::vector<T>& v2) {
  std::vector<T> v;
  v.insert(v.end(), v1.begin(), v1.end());
  v.insert(v.end(), v2.begin(), v2.end());
  return std::move(v);
}

constexpr std::vector<vertex_3D>
create_quad(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 v4, glm::vec3 color) {
  auto n1 = glm::cross(v2 - v1, v3 - v1);
  auto n2 = glm::cross(v3 - v1, v4 - v1);

  return {
      {.position = v1, .normal = n1, .uv = {0.F, 0.F}},
      {.position = v2, .normal = n1, .uv = {0.F, 1.F}},
      {.position = v3, .normal = n1, .uv = {1.F, 1.F}},
      {.position = v3, .normal = n2, .uv = {1.F, 1.F}},
      {.position = v4, .normal = n2, .uv = {1.F, 0.F}},
      {.position = v1, .normal = n2, .uv = {0.F, 0.F}}
  };
}

std::vector<vertex_3D> create_cube(glm::vec3 position, float size) {
  auto face1 = create_quad(
      position + glm::vec3{size / 2, size / 2, size / 2},
      position + glm::vec3{-size / 2, size / 2, size / 2},
      position + glm::vec3{-size / 2, -size / 2, size / 2},
      position + glm::vec3{size / 2, -size / 2, size / 2},
      {1., 1., 1.}
  );
  auto face2 = create_quad(
      position + glm::vec3{size / 2, size / 2, -size / 2},
      position + glm::vec3{-size / 2, size / 2, -size / 2},
      position + glm::vec3{-size / 2, -size / 2, -size / 2},
      position + glm::vec3{size / 2, -size / 2, -size / 2},
      {1., 1., 1.}
  );
  auto face3 = create_quad(
      position + glm::vec3{size / 2, size / 2, size / 2},
      position + glm::vec3{size / 2, -size / 2, size / 2},
      position + glm::vec3{size / 2, -size / 2, -size / 2},
      position + glm::vec3{size / 2, size / 2, -size / 2},
      {1., 1., 1.}
  );
  auto face4 = create_quad(
      position + glm::vec3{-size / 2, size / 2, size / 2},
      position + glm::vec3{-size / 2, size / 2, -size / 2},
      position + glm::vec3{-size / 2, -size / 2, -size / 2},
      position + glm::vec3{-size / 2, -size / 2, size / 2},
      {1., 1., 1.}
  );
  auto face5 = create_quad(
      position + glm::vec3{size / 2, size / 2, size / 2},
      position + glm::vec3{-size / 2, size / 2, size / 2},
      position + glm::vec3{-size / 2, size / 2, -size / 2},
      position + glm::vec3{size / 2, size / 2, -size / 2},
      {1., 1., 1.}
  );
  auto face6 = create_quad(
      position + glm::vec3{size / 2, -size / 2, size / 2},
      position + glm::vec3{size / 2, -size / 2, -size / 2},
      position + glm::vec3{-size / 2, -size / 2, -size / 2},
      position + glm::vec3{-size / 2, -size / 2, size / 2},
      {1., 1., 1.}
  );

  return combine(face1, combine(face2, combine(face3, combine(face4, combine(face5, face6)))));
}