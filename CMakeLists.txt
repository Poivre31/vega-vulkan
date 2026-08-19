cmake_minimum_required(VERSION 3.20)

project(VegaVulkan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(VegaVulkan)

target_sources(VegaVulkan PRIVATE src/main.cpp)

add_subdirectory(external)

target_link_libraries(VegaVulkan PRIVATE SDL)

install(TARGETS VegaVulkan DESTINATION bin)

