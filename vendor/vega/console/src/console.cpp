#include "console/console.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/common.h>

bool console::exists(const std::string& name) noexcept {
  return spdlog::get(name) != nullptr;
}

vega_console
console::create(const std::string& name, spdlog::level::level_enum level, bool silence) noexcept {
  assert(_engine_console != nullptr);
  if (exists(name)) {
    _engine_console->error(
        "Trying to create console [{:s}] that already exists.\nCheck console::exists(name) "
        "if you're not sure that console has been created.\nReturning sink console (discards all "
        "commands)",
        name
    );
    return _sink_console;
  }

  try {
    vega_console con = spdlog::stdout_color_mt(name);
    con->set_level(level);
    con->set_pattern("[%T] [%^%l%$] [%n] %v");

    if (!silence) {
      con->trace("Created console [{:s}]", name);
    }
    return con;
  } catch (...) {
    return handle_exception("Console/create");
  }
}

vega_console console::get(const std::string& name, bool silence) noexcept {
  try {
    vega_console con = spdlog::get(name);
    if (con) {
      if (!silence) {
        con->trace("Returning console [{:s}]", name);
      }
      return con;
    } else {
      _engine_console->error(
          "Console [{:s}] doesn't exist yet, returning sink console (discards all "
          "commands).\nCheck "
          "if console::exists(name) if you're not sure it has been created",
          name
      );
      return _sink_console;
    }
  } catch (...) {
    return handle_exception("Console/get");
  }
}

vega_console console::get(consoles default_console, bool silence) noexcept {
  vega_console console{};
  switch (default_console) {
    case consoles::engine:
      console = _engine_console;
      break;
    case consoles::math:
      console = _math_console;
      break;
    case consoles::timer:
      console = _timer_console;
      break;
    case consoles::test:
      console = _test_console;
      break;
    case consoles::graphics:
      console = _graphics_console;
      break;
    case consoles::assets:
      console = _assets_console;
      break;
  }
  assert(console != nullptr && "Getting a default console that hasn't been implemented or created");
  if (!silence) {
    console->trace("Found default console [{:s}]", console->name());
  }
  return console;
}

vega_console console::handle_exception(std::string_view context) noexcept {
  assert(_engine_console != nullptr);
  try {
    throw;
  } catch (const std::exception& e) {
    _engine_console->error(
        "[{}] console exception : {:s}.\nReturning sink console (discards all commands)",
        context,
        e.what()
    );
    return _sink_console;
  } catch (...) {
    _engine_console->error(
        "[{}] console unknown error.\nReturning sink console (discards all commands)", context
    );
    return _sink_console;
  }
}