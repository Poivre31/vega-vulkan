#pragma once

struct model_info {
  std::string object_folder;
  std::string mesh_name;
  float scale  = 1.F;
  bool z_is_up = true;
};

namespace meshes {

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

}  // namespace meshes
