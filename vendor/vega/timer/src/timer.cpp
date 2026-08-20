#include "timer/timer.hpp"

#include <console/console.hpp>

#include <chrono>
#include <mutex>
#include <ratio>
#include <string>
#include <thread>

using std::chrono::duration;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;

void timer::start(const std::string& name) {
  std::scoped_lock<std::mutex> lock(_mutex);

  if (is_timer_protected(name)) {
    _console->error("Trying to (re)create [global] timer");
  } else {
    _watches.insert_or_assign(name, timer_data{.t0 = steady_clock::now()});
    _console->trace("Started timer [{:s}]", name);
  }
}

void timer::pause(const std::string& name) {
  std::scoped_lock<std::mutex> lock(_mutex);

  if (is_timer_protected(name)) {
    _console->error("Trying to pause  [global] timer");
  } else if (!_watches.contains(name)) {
    _console->error("Trying to pause a timer [{:s}] that hasn't been created", name);
  } else {
    auto& watch = _watches.at(name);
    if (!watch.running) {
      _console->warn("timer has already been paused");
      return;
    }
    watch.offset  += delta_time(watch.t0, steady_clock::now(), time_unit::nanosecond);
    watch.running  = false;
    _console->trace("Paused timer [{:s}]", name);
  }
}

void timer::restart(const std::string& name) {
  std::scoped_lock<std::mutex> lock(_mutex);

  auto t = steady_clock::now();
  if (is_timer_protected(name)) {
    _console->error("Trying to restart [global] timer");
  } else if (!_watches.contains(name)) {
    _console->error("Trying to restart a timer [{:s}] that hasn't been created", name);

  } else {
    auto& watch = _watches.at(name);
    if (watch.running) {
      _console->warn("Trying to restart timer [{:s}] but it is already running", name);
    } else {
      watch.t0      = t;
      watch.running = true;
      _console->trace("Restarted timer [{:s}]", name);
    }
  }
}

void timer::reset(const std::string& name) {
  std::scoped_lock<std::mutex> lock(_mutex);

  if (is_timer_protected(name)) {
    _console->error("Trying to reset [global] timer");
  } else if (!_watches.contains(name)) {
    _console->error("Trying to reset a timer [{:s}] that hasn't been created", name);

  } else {
    _watches.at(name) = {.t0 = steady_clock::now(), .offset = 0, .running = true};
    _console->trace("Reset timer [{:s}]", name);
  }
}

void timer::destroy(const std::string& name) {
  std::scoped_lock<std::mutex> lock(_mutex);

  if (is_timer_protected(name)) {
    _console->error("Trying to destroy [global] timer");
  } else if (!_watches.contains(name)) {
    _console->error("Trying to destroy a timer [{:s}] that hasn't been created", name);

  } else {
    _watches.erase(name);
  }
}

void timer::print_elapsed_time(const std::string& name, time_unit unit, size_t precision) {
  std::scoped_lock<std::mutex> lock(_mutex);

  auto t = steady_clock::now();
  if (!_watches.contains(name)) {
    _console->error(
        "Trying to print the time of a timer [{:s}] that "
        "hasn't been created",
        name
    );
  } else {
    double dt = _watches.at(name).offset * time_unit_factor(unit);
    if (_watches.at(name).running) {
      dt += delta_time(_watches.at(name).t0, t, unit);
    }
    _console->info(
        fmt::runtime("Time since timer [{:s}] start: {:." + std::to_string(precision) + "g} {:s}"),
        name,
        dt,
        time_unit_text(unit)
    );
  }
}

double timer::get_elapsed_time(const std::string& name, time_unit unit) {
  std::scoped_lock<std::mutex> lock(_mutex);

  auto t = steady_clock::now();
  if (!_watches.contains(name)) {
    _console->error(
        "Trying to print the time of a timer [{:s}] that "
        "hasn't been created",
        name
    );
    return 0.;
  } else {
    double dt = _watches.at(name).offset * time_unit_factor(unit);
    if (_watches.at(name).running) {
      dt += delta_time(_watches.at(name).t0, t, unit);
    }
    return dt;
  }
}

void timer::stall(double time, time_unit unit) {
  auto t0           = steady_clock::now();
  auto sleep_margin = duration<double, std::milli>(100);
  auto dt           = duration<double, std::nano>(time / time_unit_factor(unit)) - sleep_margin;

  if (dt.count() > 0) {
    std::this_thread::sleep_for(dt);
  }
  while (delta_time(t0, steady_clock::now(), unit) < time) {
  }
}
