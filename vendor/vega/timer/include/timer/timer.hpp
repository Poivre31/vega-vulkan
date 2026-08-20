#pragma once
#include <console/console.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

enum class time_unit : char { second, millisecond, microsecond, nanosecond };

const time_unit default_time_unit        = time_unit::millisecond;
/** The global timer, starts at program startup and is always avalaible but
 * can't be paused, restarted or reset, always running */
const std::string protected_global_timer = "global";
/** The main timer, starts at program startup and is always avalaible */
const std::string main_timer             = "main";

/** Struct containing the time point of last timer start, an offset counter to
 * enable pausing, and a bool to keep track of paused/running state. */
struct timer_data {
  std::chrono::steady_clock::time_point t0;
  double offset = 0.;
  bool running  = true;
};

/** Checks for the global timer whose access is protected */
constexpr bool is_timer_protected(const std::string& name) {
  return name == protected_global_timer;
}

/** Returns the conversion factor from nanosecond to @param unit (eg time_in_ms
 * = time_unit_factor(time_unit::millisecond) * time_in_ns) */
constexpr double time_unit_factor(time_unit unit) noexcept {
  switch (unit) {
    case time_unit::second:
      return 1e-9;
    case time_unit::millisecond:
      return 1e-6;
    case time_unit::microsecond:
      return 1e-3;
    case time_unit::nanosecond:
      return 1e0;
  }
  std::abort();
}

/** Returns the symbol associated with unit @param unit (ms, µs...) */
constexpr std::string_view time_unit_text(time_unit unit) noexcept {
  switch (unit) {
    case time_unit::second:
      return "s";
    case time_unit::millisecond:
      return "ms";
    case time_unit::microsecond:
      return "µs";
    case time_unit::nanosecond:
      return "ns";
  }
  std::abort();
}

/**
 * @brief Returns time difference between @param t1 and @param t2 in given time
 * unit @param unit
 *
 * @return double
 */
[[nodiscard]] constexpr double delta_time(
    std::chrono::steady_clock::time_point t1,
    std::chrono::steady_clock::time_point t2,
    time_unit unit
) noexcept {
  auto delta = static_cast<double>(duration_cast<std::chrono::nanoseconds>(t2 - t1).count());
  return delta * time_unit_factor(unit);
}

/**
 * @brief Timer class to manage multiple watches.
 * Start a timer with `start`, pause it with `pause`, start it again with
 `restart` and reset it to current time with `reset`.
 *
 * `time_unit` lets you define the unit (s, ms, µs, ns) you want to set or get a
 time with
 *
 * You can stall the execution of the program for a given time with `stall`
 *
 *  Status and time is printer using [VegaTimer] console which is
 available with `console::get(vega_consoles::timer)`
 *
 * [global] timer starts at program startup and can't be paused/reset.
 * [main] is available by default and can be manipulated like other timers.
 *
 */
class timer {
 public:
  timer() = delete;

  /** Creates a timer @param name at this instant */
  static void start(const std::string& name = main_timer);

  /** Pauses the execution of timer @param name until restart, keeping track
   * of time spent until this instant */
  static void pause(const std::string& name = main_timer);

  /** Unpause a paused timer */
  static void restart(const std::string& name = main_timer);

  /** Same as start() but doesn't create the timer if it doesn't exist, use
   * this rather than start() when you know the timer has already been
   * created */
  static void reset(const std::string& name = main_timer);

  /** Erase a timer from memory */
  static void destroy(const std::string& name = main_timer);

  /** Prints time elapsed since timer creating, excluding time spent paused */
  static void print_elapsed_time(
      const std::string& name = main_timer,
      time_unit unit          = default_time_unit,
      size_t precision        = 4
  );

  /** Returns time elapsed since timer creating, excluding time spent paused
   */
  [[nodiscard]] static double
  get_elapsed_time(const std::string& name = main_timer, time_unit unit = default_time_unit);

  /** Stops the execution of all threads until @param time has passed */
  static void stall(double time, time_unit unit = time_unit::second);

 private:
  static inline std::mutex _mutex;

  static inline vega_console _console = console::get(consoles::timer);

  static inline std::unordered_map<std::string, timer_data> _watches{
      std::pair(protected_global_timer, timer_data{.t0 = std::chrono::steady_clock::now()}),
      std::pair(main_timer, timer_data{.t0 = std::chrono::steady_clock::now()})
  };
};
