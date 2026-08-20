if(BUILD_TESTING)
    add_executable(timer_test test_timer.cpp)
    target_link_libraries(timer_test PRIVATE GTest::gtest_main)
    target_link_libraries(timer_test PRIVATE Timer)

    include(GoogleTest)
    gtest_discover_tests(timer_test)
endif()