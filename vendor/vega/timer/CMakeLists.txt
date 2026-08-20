add_library(Timer
    "src/timer.cpp"
    "include/timer/timer.hpp"
)

target_include_directories(Timer PUBLIC
    "include"
)

target_link_libraries(Timer PUBLIC Console)

if(MSVC)
    target_compile_options(Timer PRIVATE -Wall -W4)
else()
    target_compile_options(Timer PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()
