#pragma once
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/null_sink.h>

#include <cstdint>
#include <memory>
#include <string>

#include "console/format.hpp"  // IWYU pragma: export

constexpr bool silence_color_management = true;

namespace level {

using spdlog::level::critical;
using spdlog::level::debug;
using spdlog::level::err;
using spdlog::level::info;
using spdlog::level::trace;
using spdlog::level::warn;

}  // namespace level

/** The engine's consoles that are created at startup and always available. */
enum class consoles : uint8_t { engine, math, timer, test, graphics, assets };

using vega_console = std::shared_ptr<spdlog::logger>;

/**
 * @brief Console class to manage spdlog loggers
 * [VegaEngine] console is always available and the default return to `sget`
 *
 */
class console {
 public:
  /**
   * @brief Checks if a console named @param name already exists. Should be called before console
   * creation/get if you're not sure a console hasn't been created before
   */
  [[nodiscard]] static bool exists(const std::string& name = "VegaEngine") noexcept;

  /**
   * @brief Creates a console called @param name. Error if console with the same name already
   * exists, returns sink console (discards all commands).
   *
   * @param log_level Console's log level, from trace to critical, defaults to trace
   * @param silence Disable printing trace logs when creating console
   */
  static vega_console create(
      const std::string& name,
      spdlog::level::level_enum log_level = level::trace,
      bool silence                        = silence_color_management
  ) noexcept;

  /**
  * @brief Returns a console called @param name. Error if it doesn't exist
  yet, returns sink console (discards all commands).
  */
  [[nodiscard]] static vega_console
  get(const std::string& name, bool silence = silence_color_management) noexcept;

  /**
   * @brief Returns one of the default consoles such as [VegaEngine] or
   * [VegaMath].
   */
  [[nodiscard]] static vega_console
  get(consoles console, bool silence = silence_color_management) noexcept;

 private:
  static inline auto _engine_console = create("VegaEngine", level::trace, silence_color_management);
  static inline auto _math_console   = create("VegaMath", level::trace, silence_color_management);
  static inline auto _timer_console  = create("VegaTimer", level::trace, silence_color_management);
  static inline auto _test_console   = create("VegaTest", level::trace, silence_color_management);
  static inline auto _graphics_console =
      create("VegaGraphics", level::trace, silence_color_management);
  static inline auto _assets_console = create("VegaAssets", level::trace, silence_color_management);

  static inline auto _sink_console =
      std::make_shared<spdlog::logger>("Sink", std::make_shared<spdlog::sinks::null_sink_mt>());
};

/** @brief  */
template <typename... Args>
static inline void vega_assert(
    bool expression,
    const vega_console& console,
    spdlog::format_string_t<Args...> message,
    Args&&... args
) {
  if (!expression) {
    console->critical(message, std::forward<Args>(args)...);
  }
  assert(expression);
}