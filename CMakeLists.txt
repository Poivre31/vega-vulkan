cmake_minimum_required(VERSION 3.26)

project(VegaVulkan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(VegaVulkan)

target_sources(VegaVulkan PRIVATE
    "src/main.cpp"

    "include/graphics.hpp"
    "include/glm.hpp"
    "include/stb_image.hpp"
    "include/sdl.hpp"
    "include/layer.hpp"
    "include/application.hpp"
    include/vulkan/vulkan_layer.hpp
)
target_include_directories(VegaVulkan PRIVATE "include")

add_subdirectory(vendor)

find_package(Vulkan 1.3 REQUIRED)
target_compile_definitions(VegaVulkan PRIVATE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_precompile_headers(VegaVulkan PRIVATE "include/glm.hpp" "include/graphics.hpp")

target_link_libraries(VegaVulkan PRIVATE
    vendor
    Vulkan::Vulkan
)

add_custom_target(CopyShaders
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying resources: ${CMAKE_SOURCE_DIR}/resources to ${CMAKE_BINARY_DIR}/resources"
)

add_dependencies(VegaVulkan CopyShaders)

install(TARGETS VegaVulkan DESTINATION .)
install(DIRECTORY resources DESTINATION .)
