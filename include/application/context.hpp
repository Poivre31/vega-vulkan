#pragma once

#include <SDL3/SDL.h>

#include "application/config.hpp"
#include "application/scene.hpp"

#include "player/camera.hpp"

#include "graphics/context.hpp"

struct application_context {
  SDL_Window* window = nullptr;

  int width    = config::startup_width;
  int height   = config::startup_height;
  bool running = false;

  double time                   = 0.;
  double avg_dt                 = 0.;
  double last_fixed_update_time = 0.;
  double last_dt_window_time    = 0.;
  double fixed_time             = 0.;

  uint64_t frame       = 0;
  uint64_t fixed_frame = 0;

  Camera* active_camera = nullptr;
  struct {
    float x;
    float y;
    float dx;
    float dy;
  } mouse_data{};

  vulkan_context vulkan;

  //   resources_data resources;
  scene active_scene;

  std::vector<std::vector<char>> shader_modules;
  bool recompile_shaders    = false;
  bool frame_buffer_resized = false;
};