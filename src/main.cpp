#include <console/console.hpp>
#include "application.hpp"

class fps_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  void init() noexcept final {}
  void update(double dt) noexcept final {}
  void fixed_update(double dt) noexcept final {
    _console->info(
        "Physics at {:.2f}fps, rendering at avg {:.2f}fps", 1. / dt, 1. / get_app_context()->avg_dt
    );
  }
  void cleanup() noexcept final {}

 private:
  vega_console _console = console::create("FPS");
};

int main() {
  console::set_library_consoles_log_level(level::debug);
  application<fps_layer> app("App");
  app.run();
}