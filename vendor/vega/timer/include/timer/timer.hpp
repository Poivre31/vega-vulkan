#pragma once
#include <console/console.hpp>

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

enum class time_unit : char { second, millisecond, microsecond, nanosecond };

const time_unit default_time_unit = time_unit::millisecond;

/** The global timer, starts at program startup and is always avalaible but
 * can't be paused, restarted or reset, always running */
const std::string protected_global_timer = "global";

/** The main timer, starts at program startup and is always avalaible */
const std::string main_timer = "main";

/** Struct containing the time point of last timer start, an offset counter to
 * enable pausing, and a bool to keep track of paused/running state. */
struct timer_data {
  std::chrono::steady_clock::time_point t0;
  double offset = 0.;
  bool running  = true;
};

/** Checks for the global timer whose access is protected */
constexpr bool is_timer_protected(std::string_view name) noexcept {
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
}

/** Returns the symbol associated with unit @param unit (ms, µs...) */
constexpr std::string time_unit_text(time_unit unit) noexcept {
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

  /** Checks wether a timer named @param name has already been created */
  [[nodiscard]] static bool exists(const std::string& name) noexcept;

  /** Creates a timer @param name at this instant, does nothing if timer with name @param name
   * already exists */
  static void create(const std::string& name) noexcept;

  /** Pauses the execution of timer @param name until restart, keeping track
   * of time spent until this instant */
  static void pause(const std::string& name = main_timer) noexcept;

  /** Resume the execution a paused timer */
  static void resume(const std::string& name = main_timer) noexcept;

  /** Recreates timer @param name at this instant, does nothing if it doesn't exist*/
  static void reset(const std::string& name = main_timer) noexcept;

  /** Erase a timer from memory */
  static void destroy(const std::string& name = main_timer) noexcept;

  /** Prints time elapsed since timer creation, excluding time spent paused */
  static void print_elapsed_time(
      const std::string& name = main_timer,
      time_unit unit          = default_time_unit,
      size_t precision        = 0
  ) noexcept;

  /** Logs "message + time/unit" to given console*/
  static void log_time_to_console(
      const std::string& name,
      const vega_console& console,
      std::string_view message,
      time_unit unit   = default_time_unit,
      size_t precision = 0
  ) noexcept;

  /** Prints time elapsed since timer creation, excluding time spent paused, for each timer still
   * existing TO OPTIMIZE*/
  static void
  print_all_elapsed_times(time_unit unit = default_time_unit, size_t precision = 0) noexcept;

  /** Returns time elapsed since timer creation, excluding time spent paused */
  [[nodiscard]] static double get_elapsed_time(
      const std::string& name = main_timer,
      time_unit unit          = default_time_unit
  ) noexcept;

  /** Stops the execution of all threads until @param time has passed */
  static void stall(double time, time_unit unit = time_unit::second) noexcept;

 private:
  static void handle_exception(std::string_view context) noexcept;

  static inline std::mutex _mutex;

  static inline vega_console _console = console::get(consoles::timer);

  static inline std::unordered_map<std::string, timer_data> _watches{
      std::pair(protected_global_timer, timer_data{.t0 = std::chrono::steady_clock::now()}),
      std::pair(main_timer, timer_data{.t0 = std::chrono::steady_clock::now()})
  };
};

class scoped_timer {
  // #ifndef NDEBUG
 public:
  scoped_timer(std::string_view name) : _name(name) {
    if (!timer::exists(_name)) {
      timer::create(_name);
    } else {
      timer::resume(_name);
    }
  }
  ~scoped_timer() { timer::pause(_name); }

  scoped_timer(const scoped_timer&)       = delete;
  scoped_timer(scoped_timer&&)            = delete;
  scoped_timer& operator=(scoped_timer)   = delete;
  scoped_timer& operator=(scoped_timer&&) = delete;

 private:
  std::string _name;
  // #else
  //  public:
  //   scoped_timer(std::string_view) {}
  // #endif
};