#pragma once
#include <SDL3/SDL.h>
#include "camera_class.hpp"

struct application_context {
  SDL_Window* window = nullptr;

  int width    = 1280;
  int height   = 720;
  bool running = false;

  double time                   = 0.;
  double avg_dt                 = 0.;
  double last_fixed_update_time = 0.;
  double last_dt_window_time    = 0.;

  uint64_t frame = 0;

  Camera* active_camera = nullptr;
  struct {
    float x;
    float y;
    float dx;
    float dy;
  } mouse_data{};
};

class Ilayer {
 public:
  Ilayer(application_context* context) : _context(context) {
    assert(context != nullptr && "A valid application context should always be passed");
  }
  virtual ~Ilayer()                = default;
  Ilayer(const Ilayer&)            = default;
  Ilayer(Ilayer&&)                 = default;
  Ilayer& operator=(const Ilayer&) = default;
  Ilayer& operator=(Ilayer&&)      = default;

  virtual void init() noexcept            = 0;
  virtual void update(double dt) noexcept = 0;
  virtual void fixed_update(double time_step) noexcept {};
  virtual void cleanup() noexcept = 0;

  [[nodiscard]] application_context* get_app_context() const noexcept { return _context; }

 private:
  application_context* _context;
};