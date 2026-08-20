#include <console/console.h>
#include <gtest/gtest.h>

#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>

TEST(TestConsole, TestLogLevels) {
  EXPECT_NO_FATAL_FAILURE(
      auto test_console = console::create("OrionConsole"); test_console->set_level(level::trace);
      test_console->info("The integral of sin between 0 and pi is around : {:.4f}", 2.);
      test_console->debug("Successeful integration !");

      double a = 2;

      test_console->trace("This is a message... {:s}", "(trace)");
      test_console->debug("This is a message... {:s}", "(debug)");
      test_console->info("This is a message... {:s}", "(info)");
      test_console->warn("This is a message... {:s}", "(warn)");
      test_console->error("This is a message... {:s}", "(error)");
      test_console->critical("This is a message... {:s}", "(critical)");
      test_console->info("This is a number... {:.5f}", std::numbers::pi);
      test_console->info("This is a number... {:+.5g}", std::numbers::pi);
  );

  EXPECT_NO_FATAL_FAILURE(
      auto console = console::get(); console->set_level(level::warn);
      std::cout << "Changed log level to 'warn'\n";
      console->trace("This is a message... {:s}", "(trace)");
      console->debug("This is a message... {:s}", "(debug)");
      console->info("This is a message... {:s}", "(info)");
      console->warn("This is a message... {:s}", "(warn)");
      console->error("This is a message... {:s}", "(error)");
      console->critical("This is a message... {:s}", "(critical)");
  );
}

TEST(TestConsole, TestConsoleAccess) {
  EXPECT_NO_FATAL_FAILURE(
      auto console = console::get(); console = console::create_or_get("VegaEngine");
      auto test_conole                       = console::create_or_get("Test");
      auto test2                             = console::get("Test");
      test_conole->debug("Working 1");
      test2->debug("Working 2");
  );

  EXPECT_THROW(auto c = console::get("Lame"), std::invalid_argument);
  EXPECT_THROW(console::create("VegaEngine"), std::invalid_argument);
}
