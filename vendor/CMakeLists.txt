set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/_deps")
include (FetchContent)

add_library(vendor INTERFACE)

# VEGA
add_subdirectory(spdlog)
add_subdirectory(vega/console)
add_subdirectory(vega/timer)
# END VEGA

# VULKAN
# FOR LINUX : install libvulkan-dev vulkan-validationlayers mesa-vulkan-drivers vulkan-tools
if (UNIX)
find_library(VULKAN_LIB vulkan REQUIRED)
target_include_directories(vendor INTERFACE vulkan-headers/include)
target_link_libraries(vendor INTERFACE ${VULKAN_LIB})

# FOR WINDOWS : install lunar SDK
elseif(WIN32)
find_package(Vulkan 1.3 REQUIRED)
target_link_libraries(vendor INTERFACE Vulkan::Vulkan)

endif()

target_compile_definitions(vendor INTERFACE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)
# END VULKAN

# SLANG
set(SLANG_VERSION "2026.16")

if(WIN32)
    message(IMPLEMENT WINDOWS)
elseif(UNIX)
    set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-linux-x86_64.zip")
endif()

FetchContent_Declare(slang_bin URL ${SLANG_URL})
FetchContent_MakeAvailable(slang_bin)

if(UNIX)
    set(SLANG_SHARED_LIB "${slang_bin_SOURCE_DIR}/lib/libslang-compiler.so.0.${SLANG_VERSION}")
endif()

add_library(slang SHARED IMPORTED)
set_target_properties(slang PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${slang_bin_SOURCE_DIR}/include"
    IMPORTED_LOCATION "${SLANG_SHARED_LIB}"
)

add_custom_target(CopySlangLib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SLANG_SHARED_LIB} ${CMAKE_BINARY_DIR}
)
add_dependencies(vendor CopySlangLib)


install(PROGRAMS ${SLANG_SHARED_LIB}
    DESTINATION .
)
# END SLANG

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
# cmake -B build -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=0 -DSDL_STATIC=1 -DSDL_TEST=0 -DSDL_GPU=0 -DSDL_RENDER=0 -DSDL_CAMERA=0 -DSDL_JOYSTICK=0 -DSDL_HAPTIC=0 -DSDL_POWER=0 -DSDL_SENSOR=0
# cmake --build build
# set(SDL_SHARED OFF  CACHE BOOL "" FORCE)
# set(SDL_STATIC ON CACHE BOOL "" FORCE)
# set(SDL_TEST   OFF CACHE BOOL "" FORCE)

# set(SDL_JOYSTICK  OFF CACHE BOOL "" FORCE)
# set(SDL_HAPTIC  OFF CACHE BOOL "" FORCE)
# set(SDL_POWER  OFF CACHE BOOL "" FORCE)
# set(SDL_SENSOR  OFF CACHE BOOL "" FORCE)
# set(SDL_CAMERA   OFF CACHE BOOL "" FORCE)
# set(SDL_AUDIO   OFF CACHE BOOL "" FORCE)

# set(SDL_GPU  OFF CACHE BOOL "" FORCE)
# set(SDL_RENDER   OFF CACHE BOOL "" FORCE)

# set(SDL_OPENGL   OFF CACHE BOOL "" FORCE)
# set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
# set(SDL_METAL     OFF CACHE BOOL "" FORCE)
# set(SDL_DIRECT3D  OFF CACHE BOOL "" FORCE)
# set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)

# set(SDL_VULKAN ON CACHE BOOL "" FORCE)

# add_subdirectory(SDL3)
add_library(sdl STATIC IMPORTED)
if(UNIX)
    set_target_properties(sdl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/vendor/SDL3-bin/include"
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/SDL3-bin/lib/libSDL3.a"
    )
endif()

# END SDL3

target_link_libraries(vendor INTERFACE
    # SDL3::SDL3
    slang
    sdl
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    glm::glm
    Console
    Timer
)