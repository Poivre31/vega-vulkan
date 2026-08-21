#include "graphics.hpp"
#include <console/console.hpp>
#include "application.hpp"

const vk::ApplicationInfo appInfo{
    .pApplicationName   = "Vega Vulkan",
    .applicationVersion = vk::makeVersion(0, 1, 0),
    .apiVersion         = vk::ApiVersion13
};

class log_layer : public Ilayer {
 public:
  void init() noexcept final {}
  void update(double dt) noexcept final {
    _console->info("Frame {}, delta time is {:.2g}ms", frame, dt * 1e3);
    frame++;
  }
  void cleanup() noexcept final {}

 private:
  vega_console _console = console::create("DeltaTimePrinter");
  uint64_t frame        = 0;
};

int main() {
  console::set_library_consoles_log_level(level::debug);
  application<log_layer> app("SDL app");
  app.run();
}