#pragma once
#include "layer.hpp"
#include "sdl.hpp"
#include "camera.hpp"
#include "vulkan/vulkan_layer.hpp"
#include "assets.hpp"
#include <console/console.hpp>
#include <concepts>
#include <queue>

template <class T>
concept application_layer = std::derived_from<T, Ilayer> && std::is_final_v<T>;

namespace application_config {

constexpr double average_dt_window_length = 1.;

constexpr double fixed_time_step = 1. / 60;

};  // namespace application_config

template <application_layer... layers>
class application {
 public:
  application(std::string_view name) : _name(name) {
    _console = console::create(_name);
    push_layer<sdl_layer>();
    push_layer<camera_layer>();
    push_layer<vulkan_layer>();
    push_layer<assets_layer>();
    (push_layer<layers>(), ...);
  }
  ~application() {
    while (!_layers.empty()) {
      _layers.pop_back();
    }
  }

  application(application&)                  = delete;
  application(application&&)                 = delete;
  application& operator=(const application&) = delete;
  application& operator=(application&&)      = delete;

  void run() noexcept {
    timer::create(_name);
    int last_initialised_layer = 0;
    bool initialisation_failed = false;
    for (auto& layer : _layers) {
      if (!layer->init()) {
        initialisation_failed = true;
        break;
      }
      last_initialised_layer++;
    }
    if (initialisation_failed) {
      for (int i = last_initialised_layer; i >= 0; i--) {
        _layers[i]->cleanup();
      }
      return;
    }

    _context.running = true;
    timer::log_time_to_console(
        _name, _console, "Initialised application in", time_unit::millisecond
    );
    timer::create("runtime");

    if (!is_context_valid(_context)) {
      _console->error(
          "Not all context resources have been assigned, aborting to avoid null ptr dereferencing"
      );
      _context.running = false;
    }

    timer::create("frame-time");
    _frame_times.push(_context.time);
    double delta_time = 0.;
    while (_context.running) {
      // TIMING
      if (_context.frame > 0) {
        _context.time = timer::get_elapsed_time("runtime");
        _frame_times.push(_context.time);
        while (_frame_times.back() - _frame_times.front()
               > application_config::average_dt_window_length) {
          _context.last_dt_window_time = _frame_times.front();
          _frame_times.pop();
        }
        assert(_frame_times.size() > 0);
        _context.avg_dt = (_frame_times.back() - _context.last_dt_window_time)
                          / static_cast<double>(_frame_times.size());
        delta_time      = timer::get_elapsed_time("frame-time", time_unit::second);
      }

      // FRAME UPDATE
      timer::reset("frame-time");
      for (auto& layer : _layers) {
        layer->update(delta_time);
      }

      // FIXED UPDATE
      double fixed_dt = _context.time - _context.last_fixed_update_time;
      if (fixed_dt >= application_config::fixed_time_step) {
        for (auto& layer : _layers) {
          layer->fixed_update(application_config::fixed_time_step);
        }
        _context.last_fixed_update_time = _context.time;
        _context.fixed_frame++;
        _context.fixed_time = application_config::fixed_time_step * _context.fixed_frame;
      }

      _context.frame++;
    }

    for (auto& layer : _layers) {
      layer->cleanup();
    }
  }

 private:
  template <application_layer T>
  void push_layer() {
    _layers.push_back(std::make_unique<T>(&_context));
  }

  std::vector<std::unique_ptr<Ilayer>> _layers;
  std::string _name;
  vega_console _console;
  application_context _context;

  std::queue<double> _frame_times;
};