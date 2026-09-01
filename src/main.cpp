#include <console/console.hpp>
#include "application/application.hpp"
#include "timer/timer.hpp"

class fps_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  bool init() noexcept final { return true; }
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
  application<> app("App");
  app.run();
  timer::print_all_elapsed_times(time_unit::millisecond);
}