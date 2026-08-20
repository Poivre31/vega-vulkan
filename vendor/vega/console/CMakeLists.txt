add_library(Console
    "include/console/format.hpp"

    "src/console.cpp"
    "include/console/console.hpp"
)

# target_compile_definitions(Console PUBLIC "SILENCE_CONSOLE_MANAGEMENT=$<IF:$<CONFIG:Debug>,0,1>")
target_link_libraries(Console PUBLIC spdlog::spdlog)

target_include_directories(Console PUBLIC "include")

# Default logging behaviour of console manager: if silence_console_management
# is set to false, consoles will print trace messages when created, found...
# Setting it to true only shows errors and such

if(MSVC)
    target_compile_options(Console PRIVATE -Wall -W4)
else()
    target_compile_options(Console PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()
