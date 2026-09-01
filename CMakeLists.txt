cmake_minimum_required(VERSION 3.26)

project(VegaVulkan)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")

add_executable(VegaVulkan)

# target_compile_options(VegaVulkan PRIVATE -fsanitize=address)
# target_link_options(VegaVulkan PRIVATE -fsanitize=address)

target_sources(VegaVulkan PRIVATE
    "src/main.cpp"

    "include/glm.hpp"
    "include/io.hpp"

    "include/application/application.hpp"
    "include/application/config.hpp"
    "include/application/context.hpp"
    "include/application/layer.hpp"
    "include/application/scene.hpp"
    "include/application/sdl.hpp"

    "include/graphics/buffer.hpp"
    "include/graphics/config.hpp"
    "include/graphics/context.hpp"
    "include/graphics/gpu_objects.hpp"
    "include/graphics/graphics.hpp"
    "include/graphics/image.hpp"
    "include/graphics/single_command_buffer.hpp"
    "include/graphics/slang_compiler.hpp"
    "include/graphics/vulkan_layer.hpp"

    "include/player/camera.hpp"
    "include/player/camera_layer.hpp"

    "include/resources/allocator.hpp"
    "include/resources/assets.hpp"
    "include/resources/handle.hpp"
    "include/resources/material.hpp"
    "include/resources/mesh_imports.hpp"
    "include/resources/mesh.hpp"
    "include/resources/object_loader.hpp"
    "include/resources/primitive_meshes.hpp"
    "include/resources/stb_image.hpp"
    "include/resources/texture_imports.hpp"
)
target_include_directories(VegaVulkan PRIVATE "include")

add_subdirectory(vendor)

target_compile_options(VegaVulkan PRIVATE -march=x86-64 -mtune=generic)

target_precompile_headers(VegaVulkan PRIVATE "include/glm.hpp" "include/graphics/graphics.hpp")

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
