#include "timer/timer.hpp"

#include <console/console.hpp>

#include <chrono>
#include <mutex>
#include <string>

using std::chrono::duration;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;

bool timer::exists(const std::string& name) noexcept {
  return _watches.contains(name);
}

void timer::create(const std::string& name) noexcept {
  try {
    std::scoped_lock<std::mutex> lock(_mutex);

    if (is_timer_protected(name)) {
      _console->warn("Trying to (re)create [global] timer which is protected, doing nothing");
      return;
    } else if (exists(name)) {
      _console->trace("Trying to create a timer [{:s}] that already exists, doing nothing", name);
      return;
    }

    _watches.emplace(name, timer_data{.t0 = steady_clock::now()});

    _console->trace("Started timer [{:s}]", name);
  } catch (...) {
    handle_exception("Timer/creation");
  }
}

void timer::pause(const std::string& name) noexcept {
  try {
    std::scoped_lock<std::mutex> lock(_mutex);

    if (is_timer_protected(name)) {
      _console->warn("Trying to pause [global] timer which is protected, doing nothing");
      return;
    } else if (!exists(name)) {
      _console->warn(
          "Trying to pause a timer [{:s}] that hasn't been created, doing nothing", name
      );
      return;
    }

    auto& watch = _watches.at(name);
    if (!watch.running) {
      _console->trace("Timer [{:s}] has already been paused, doing nothing", name);
      return;
    }
    watch.offset  += delta_time(watch.t0, steady_clock::now(), time_unit::nanosecond);
    watch.running  = false;
    watch.t0       = steady_clock::now();
    _console->trace("Paused timer [{:s}]", name);
  } catch (...) {
    handle_exception("Timer/pause");
  }
}

void timer::resume(const std::string& name) noexcept {
  try {
    std::scoped_lock<std::mutex> lock(_mutex);

    if (is_timer_protected(name)) {
      _console->warn("Trying to restart [global] timer which is protected, doing nothing");
      return;
    } else if (!exists(name)) {
      _console->warn(
          "Trying to restart a timer [{:s}] that hasn't been created, doing nothing", name
      );
      return;
    }

    auto& watch = _watches.at(name);
    if (watch.running) {
      _console->trace(
          "Trying to restart timer [{:s}] but it is already running, doing nothing", name
      );
      return;
    }
    // double dt     = delta_time(watch.t0, steady_clock::now(), default_time_unit);
    watch.t0      = steady_clock::now();
    watch.running = true;
    _console->trace("Resumed timer [{:s}]", name);
  } catch (...) {
    handle_exception("Timer/resume");
  }
}

void timer::reset(const std::string& name) noexcept {
  try {
    std::scoped_lock<std::mutex> lock(_mutex);

    if (is_timer_protected(name)) {
      _console->warn("Trying to reset [global] timer which is protected, doing nothing");
      return;
    } else if (!exists(name)) {
      _console->warn(
          "Trying to reset a timer [{:s}] that hasn't been created, doing nothing", name
      );
      return;
    }

    _watches.at(name) = {.t0 = steady_clock::now(), .offset = 0, .running = true};
    _console->trace("Reset timer [{:s}]", name);
  } catch (...) {
    handle_exception("Timer/reset");
  }
}

void timer::destroy(const std::string& name) noexcept {
  try {
    std::scoped_lock<std::mutex> lock(_mutex);

    if (is_timer_protected(name)) {
      _console->warn("Trying to destroy [global] timer which is protected, doing nothing");
      return;
    } else if (!exists(name)) {
      _console->warn(
          "Trying to destroy a timer [{:s}] that hasn't been created, doing nothing", name
      );
      return;
    }

    _watches.erase(name);
  } catch (...) {
    handle_exception("Timer/destroy");
  }
}

void timer::print_elapsed_time(const std::string& name, time_unit unit, size_t precision) noexcept {
  try {
    _console->info(
        fmt::runtime("Time since timer [{:s}] start: {:." + std::to_string(precision) + "f} {:s}"),
        name,
        get_elapsed_time(name, unit),
        time_unit_text(unit)
    );
  } catch (...) {
    handle_exception("Timer/print");
  }
}

void timer::print_all_elapsed_times(time_unit unit, size_t precision) noexcept {
  for (auto& watch_pair : _watches) {
    print_elapsed_time(watch_pair.first, unit, precision);
  }
}

double timer::get_elapsed_time(const std::string& name, time_unit unit) noexcept {
  try {
    std::scoped_lock<std::mutex> lock(_mutex);

    if (!exists(name)) {
      _console->error(
          "Trying to print the time of a timer [{:s}] that "
          "hasn't been created, returning 0.0s",
          name
      );
      return 0.;
    }

    auto watch = _watches.at(name);
    double dt  = watch.offset * time_unit_factor(unit);
    if (watch.running) {
      dt += delta_time(watch.t0, steady_clock::now(), unit);
    }
    return dt;
  } catch (...) {
    handle_exception("Timer/getTime");
    return 0.;
  }
}

void timer::stall(double time, time_unit unit) noexcept {
  auto t0 = steady_clock::now();
  while (delta_time(t0, steady_clock::now(), unit) < time) {
    std::this_thread::yield();
  }
}

void timer::handle_exception(std::string_view context) noexcept {
  assert(_console != nullptr);
  try {
    throw;
  } catch (const std::exception& e) {
    _console->error("[{}] : timer exception : {}", context, e.what());
  } catch (...) {
    _console->error("[{}] : timer unknown error", context);
  }
}