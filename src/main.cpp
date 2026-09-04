#include <console/console.hpp>
#include "application/application.hpp"
#include "timer/timer.hpp"

#include "imgui.h"

class fps_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  void fixed_update(double dt) noexcept final {
    _console->info(
        "Physics at {:.2f}fps, rendering at avg {:.2f}fps", 1. / dt, 1. / get_app_context()->avg_dt
    );
  }

 private:
  vega_console _console = console::create("FPS");
};

class ui_layer final : public Ilayer {
 public:
  using Ilayer::Ilayer;
  void gui_update() noexcept final {
    ImGui::Begin("Debug");
    if (ImGui::Button("Press me")) {
      console::get(consoles::engine)->info("Hello!");
    }
    ImGui::End();
  }
};

int main() {
  application<ui_layer> app("App");
  app.run();
  timer::print_all_elapsed_times(time_unit::millisecond);
}