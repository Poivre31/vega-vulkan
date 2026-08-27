add_library(vendor INTERFACE)

# VEGA
add_subdirectory(spdlog)
add_subdirectory(vega/console)
add_subdirectory(vega/timer)
# END VEGA

# VULKAN
# FOR LINUX : install libvulkan-dev vulkan-validationlayers mesa-vulkan-drivers vulkan-tools
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
find_library(VULKAN_LIB vulkan REQUIRED)
target_include_directories(vendor INTERFACE vulkan-headers/include)
target_link_libraries(vendor INTERFACE ${VULKAN_LIB})

# FOR WINDOWS : install lunar SDK
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
find_package(Vulkan 1.3 REQUIRED)
target_link_libraries(vendor INTERFACE Vulkan::Vulkan)

endif()

target_compile_definitions(vendor INTERFACE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)
# END VULKAN

# VMA
add_subdirectory(vma-hpp)
target_include_directories(vendor INTERFACE vma-hpp/VulkanMemoryAllocator/include)

target_compile_definitions(vendor INTERFACE VMA_IMPLEMENTATION )
# END VMA

# GLM
add_subdirectory(GLM)

target_compile_definitions(vendor INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)
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

target_compile_definitions(vendor INTERFACE
    TINYOBJLOADER_IMPLEMENTATION
    TINYOBJLOADER_USE_MAPBOX_EARCUT
    TINYOBJLOADER_STREAM_READER_MAX_BYTES=-1
)

# END OBJ

# SDL3
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