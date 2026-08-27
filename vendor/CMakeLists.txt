add_library(vendor INTERFACE)

# VEGA
add_subdirectory(spdlog)
add_subdirectory(vega/console)
add_subdirectory(vega/timer)
# END VEGA

# VULKAN
# FOR LINUX : libvulkan-dev vulkan-validationlayers mesa-vulkan-drivers vulkan-tools
find_library(VULKAN_LIB vulkan REQUIRED)
target_compile_definitions(vendor INTERFACE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)
target_include_directories(vendor INTERFACE vulkan-headers/include)  # your vendored headers, unchanged
target_link_libraries(vendor INTERFACE ${VULKAN_LIB})
# END VULKAN

# VMA
add_subdirectory(vma-hpp)
target_include_directories(vendor INTERFACE vma-hpp/VulkanMemoryAllocator/include)
target_link_libraries(vendor INTERFACE VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp)
target_compile_definitions(vendor INTERFACE VMA_IMPLEMENTATION )
# END VMA

# GLM
target_compile_definitions(vendor INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)
add_subdirectory(GLM)
# END GLM

# STB
target_include_directories(vendor INTERFACE stb-image)
target_compile_definitions(vendor INTERFACE STB_IMAGE_IMPLEMENTATION )
# END STB

# GLTF
target_include_directories(vendor INTERFACE tinygltf)
target_compile_definitions(vendor INTERFACE TINYGLTF_IMPLEMENTATION )
# END GLTF

# OBJ
target_include_directories(vendor INTERFACE tinyobj)
target_compile_definitions(vendor INTERFACE TINYOBJLOADER_IMPLEMENTATION TINYOBJLOADER_USE_MAPBOX_EARCUT)
# END OBJ

# SDL3
if(SDL3_ALREADY_CONFIGURED)
    return()
endif()

set(SDL_SHARED OFF  CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST   OFF CACHE BOOL "" FORCE)

set(SDL_JOYSTICK  OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC  OFF CACHE BOOL "" FORCE)
set(SDL_POWER  OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR  OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA   OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO   OFF CACHE BOOL "" FORCE)

set(SDL_GPU  OFF CACHE BOOL "" FORCE)
set(SDL_RENDER   OFF CACHE BOOL "" FORCE)

set(SDL_OPENGL   OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_METAL     OFF CACHE BOOL "" FORCE)
set(SDL_DIRECT3D  OFF CACHE BOOL "" FORCE)
set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)

set(SDL_VULKAN ON CACHE BOOL "" FORCE)

add_subdirectory(SDL3)
# END SDL3

target_link_libraries(vendor INTERFACE
    SDL3::SDL3
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    glm::glm
    Console
    Timer
)



# if(WIN32)
#     add_custom_command(TARGET external POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_FILE:SDL3::SDL3>
#             $<TARGET_FILE_DIR:external>
#     )
# endif()