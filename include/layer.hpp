#pragma once
#include <SDL3/SDL.h>
#include "application/context.hpp"

bool is_context_valid(application_context& context) {
  return context.window && context.active_camera && context.vulkan.allocator
         && context.vulkan.command_pool && context.vulkan.device && context.vulkan.physical_device
         && context.vulkan.graphics_queue;
}

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

  virtual bool init() noexcept            = 0;
  virtual void update(double dt) noexcept = 0;
  virtual void fixed_update(double time_step) noexcept {};
  virtual void cleanup() noexcept = 0;

  [[nodiscard]] application_context* get_app_context() const noexcept { return _context; }

 private:
  application_context* _context;
};