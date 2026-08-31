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
    "include/vulkan/vulkan_layer.hpp"
    "include/camera.hpp"
    "include/camera_class.hpp"
    "include/primitive_meshes.hpp"
    "include/vulkan/single_command_buffer.hpp"
    "include/vulkan/buffer.hpp"
    "include/mesh.hpp"
    "include/assets.hpp"
    "include/vulkan_context.hpp"
    "include/image.hpp"
    "include/object_loader.hpp"
    "include/global_config.hpp"
    "include/slang_compiler.hpp"
    "include/io.hpp"
    "include/resources/mesh_imports.hpp"
    "include/resources/texture_imports.hpp"
    "include/allocator.hpp"
    "include/material.hpp"
)
target_include_directories(VegaVulkan PRIVATE "include")

add_subdirectory(vendor)

target_compile_options(VegaVulkan PRIVATE -march=x86-64 -mtune=generic)

target_precompile_headers(VegaVulkan PRIVATE "include/glm.hpp" "include/graphics.hpp")

target_link_libraries(VegaVulkan PRIVATE
    vendor
)

set_target_properties(VegaVulkan PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH_USE_ORIGIN TRUE
)

add_custom_target(CopyShaders
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources
    COMMENT "Copying resources: ${CMAKE_SOURCE_DIR}/resources to ${CMAKE_BINARY_DIR}/resources"
)

add_dependencies(VegaVulkan CopyShaders)

install(TARGETS VegaVulkan DESTINATION .)
install(DIRECTORY resources DESTINATION .)
