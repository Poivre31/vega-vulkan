#pragma once

#include "handle.hpp"
#include <string>

class mesh_3D;
struct model_info {
  std::string object_folder;
  std::string mesh_name;
  float scale  = 1.F;
  bool z_is_up = true;
  mutable handle<mesh_3D> mesh{};
};

namespace meshes {

const model_info viking{
    .object_folder = "resources/models/",
    .mesh_name     = "viking_room.obj",
    .scale         = 1.F,
    .z_is_up       = true,
};

const model_info tank{
    .object_folder = "resources/models/t34/",
    .mesh_name     = "t34.obj",
    .scale         = 2.F,
    .z_is_up       = false,
};

const model_info sponza{
    .object_folder = "resources/models/sponza/",
    .mesh_name     = "sponza.obj",
    .scale         = 1.F / 200,
    .z_is_up       = false,
};

const model_info tyra{
    .object_folder = "resources/models/",
    .mesh_name     = "tyra.obj",
    .scale         = 1.F,
    .z_is_up       = false,
};

}  // namespace meshes
