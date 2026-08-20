if(BUILD_TESTING)
    add_executable(console_test test_prints.cpp)

    target_link_libraries(console_test PRIVATE GTest::gtest_main)

    target_link_libraries(console_test PRIVATE Console)

    include(GoogleTest)
    gtest_discover_tests(console_test)
endif()