add_library(vendor INTERFACE)

# VEGA
add_subdirectory(spdlog)
add_subdirectory(vega/console)
add_subdirectory(vega/timer)
# END VEGA

# VMA
add_subdirectory(VMA/include)

target_compile_definitions(vendor INTERFACE VMA_IMPLEMENTATION )
# END VMA

# GLM
add_subdirectory(GLM)
# END GLM

# STB
target_include_directories(vendor INTERFACE stb-image)
target_compile_definitions(vendor INTERFACE STB_IMAGE_IMPLEMENTATION )
# END STB

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

# set(SDL_OPENGL    OFF CACHE BOOL "" FORCE)
# set(SDL_OPENGLES  OFF CACHE BOOL "" FORCE)
set(SDL_METAL     OFF CACHE BOOL "" FORCE)
set(SDL_DIRECT3D  OFF CACHE BOOL "" FORCE)
set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)

set(SDL_GPU  ON CACHE BOOL "" FORCE)
set(SDL_RENDER   ON CACHE BOOL "" FORCE)
set(SDL_OPENGL   ON CACHE BOOL "" FORCE)
set(SDL_OPENGLES ON CACHE BOOL "" FORCE)
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