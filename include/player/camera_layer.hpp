#pragma once

#include <console/console.hpp>

#include "application/layer.hpp"

#include "camera.hpp"

class camera_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;

  bool init() noexcept final {
    _cameras.emplace_back(45.F, glm::vec3{0.F, 0.F, 0.F}, glm::vec3{1.F, 0.F, 0.F});
    _active_camera = &_cameras[0];  // NOLINT

    get_app_context()->active_camera = _active_camera;

    return true;
  }
  void update(double dt) noexcept final {
    const bool* key_states = SDL_GetKeyboardState(nullptr);

    _active_camera->update(get_app_context()->width, get_app_context()->height);
    auto [forward, left, up] = _active_camera->get_view_basis();

    forward.z = 0;
    left.z    = 0;

    glm::vec<3, int> axis{};
    if (key_states[SDL_SCANCODE_W]) {
      axis.x += 1;
    }
    if (key_states[SDL_SCANCODE_S]) {
      axis.x -= 1;
    }
    if (key_states[SDL_SCANCODE_A]) {
      axis.y += 1;
    }
    if (key_states[SDL_SCANCODE_D]) {
      axis.y -= 1;
    }
    if (key_states[SDL_SCANCODE_SPACE]) {
      axis.z += 1;
    }
    if (key_states[SDL_SCANCODE_LSHIFT]) {
      axis.z -= 1;
    }
    if (axis.x != 0 || axis.y != 0 || axis.z != 0) {
      glm::vec3 direction = glm::normalize(
          (static_cast<float>(axis.x) * forward + static_cast<float>(axis.y) * left
           + static_cast<float>(axis.z) * up)
      );
      _active_camera->increment_position(_camera_speed * static_cast<float>(dt) * direction);
    }

    if (!key_states[SDL_SCANCODE_LALT]) {
      auto mouse_data = get_app_context()->mouse_data;
      double dtheta   = mouse_data.dy / static_cast<float>(get_app_context()->height)
                        * _active_camera->get_vfov();
      double dphi     = mouse_data.dx / static_cast<float>(get_app_context()->width)
                        * _active_camera->get_hfov();
      _active_camera->increment_latitude_longitude(dtheta, dphi);
      SDL_SetWindowRelativeMouseMode(get_app_context()->window, true);
    } else {
      SDL_SetWindowRelativeMouseMode(get_app_context()->window, false);
    }

    get_app_context()->active_camera = _active_camera;
  }
  void fixed_update(double time_step) noexcept final { auto pos = _active_camera->get_position(); }
  void cleanup() noexcept final {}

 private:
  std::vector<Camera> _cameras;
  Camera* _active_camera = nullptr;
  float _camera_speed    = 2.F;
  vega_console _console  = console::create("Camera");
};