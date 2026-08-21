#pragma once
#include "layer.hpp"
#include "sdl.hpp"
#include <console/console.hpp>

template <std::derived_from<Ilayer>... layers>
class application {
 public:
  application(std::string_view name) : _name(name) {
    push_layer<sdl_layer>();
    (push_layer<layers>(), ...);
    _console = console::create(_name);
  }

  template <std::derived_from<Ilayer> T>
  void push_layer() {
    _layers.push_back(std::make_unique<T>());
  }

  void run() noexcept {
    timer::create(_name);
    _console->info("Running !");
    for (auto& layer : _layers) {
      layer->init();
    }
    timer::log_time_to_console(_name, _console, "Initialised layers in");

    bool _running = true;
    SDL_Event event{0};
    timer::create("frame-time");
    while (_running) {
      while (SDL_PollEvent(&event)) {
        switch (event.type) {
          case (SDL_EVENT_QUIT):
            _running = false;
            break;

          case (SDL_EVENT_KEY_DOWN):
            if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
              _running = false;
            }
            break;
          default:
            break;
        }
      }
      SDL_zero(event);

      double dt = timer::get_elapsed_time("frame-time", time_unit::second);
      timer::reset("frame-time");
      for (auto& layer : _layers) {
        layer->update(dt);
      }
    }

    for (auto& layer : _layers) {
      layer->cleanup();
    }
  }

 private:
  std::vector<std::unique_ptr<Ilayer>> _layers;
  std::string _name;
  vega_console _console;
};