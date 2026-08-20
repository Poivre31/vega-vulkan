#include <console/console.h>
#include <gtest/gtest.h>

#include "timer/timer.h"

TEST(TestTimer, TestTimerBasics) {
    EXPECT_NO_FATAL_FAILURE(
        console::get()->info("Expecting 0.05s wait time"); timer::start();
        timer::stall(0.05); timer::print_elapsed_time();
        timer::print_elapsed_time("main", time_unit::second);
        timer::print_elapsed_time("main", time_unit::nanosecond, 8););

    EXPECT_NO_FATAL_FAILURE(console::get()->info("Expecting 9 errors");
                            timer::print_elapsed_time("nope");
                            double x = timer::get_elapsed_time("nope");
                            timer::pause("nope"); timer::restart("nope");
                            timer::start("global"); timer::pause("global");
                            timer::restart("global"); timer::reset("global");
                            timer::destroy("global"););

    timer::start("start");
    timer::stall(0.05);
    EXPECT_NEAR(timer::get_elapsed_time("start", time_unit::second), 0.05,
                0.001);
    timer::pause("start");
    timer::stall(0.1);
    EXPECT_NEAR(timer::get_elapsed_time("start", time_unit::second), 0.05,
                0.001);
    timer::pause("start");
    timer::restart("start");
    timer::stall(0.1);
    EXPECT_NEAR(timer::get_elapsed_time("start", time_unit::second), 0.15,
                0.001);

    timer::start("start");
    timer::stall(350000, time_unit::microsecond);
    EXPECT_NEAR(timer::get_elapsed_time("start", time_unit::millisecond), 350,
                5);

    EXPECT_NO_FATAL_FAILURE(timer::print_elapsed_time("global"));
}
