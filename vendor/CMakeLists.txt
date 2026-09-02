set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/_deps")
include(FetchContent)

add_library(vendor INTERFACE)

# VEGA
add_subdirectory(spdlog)
add_subdirectory(vega/console)
add_subdirectory(vega/timer)
# END VEGA

# VULKAN
# FOR LINUX : install libvulkan-dev vulkan-validationlayers mesa-vulkan-drivers vulkan-tools
if(UNIX)
    FetchContent_Declare(vulkan_headers
        GIT_REPOSITORY https://github.com/KhronosGroup/vulkan-headers
        GIT_TAG e3b1eec08173d6b825cd3ac88c885a63b621504a # 1.4.357
    )
    FetchContent_MakeAvailable(vulkan_headers)
    target_include_directories(vendor INTERFACE ${vulkan_headers_SOURCE_DIR}/include)
    find_library(VULKAN_LIB vulkan REQUIRED)
    target_link_libraries(vendor INTERFACE ${VULKAN_LIB})

    # FOR WINDOWS : install lunar SDK
elseif(WIN32)
    find_package(Vulkan 1.3 REQUIRED)
    target_link_libraries(vendor INTERFACE Vulkan::Vulkan)

endif()

target_compile_definitions(vendor INTERFACE
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)
# END VULKAN

# SLANG
set(SLANG_VERSION "2026.16")

if(WIN32)
    set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-windows-x86_64.zip")
elseif(UNIX)
    set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-linux-x86_64.zip")
endif()

message(STATUS "Fetching slang binaries...")
FetchContent_Declare(slang_bin URL ${SLANG_URL})
FetchContent_MakeAvailable(slang_bin)

if(WIN32)
    set(SLANG_SHARED_LIB "${slang_bin_SOURCE_DIR}/bin/slang-compiler.dll")
    set(SLANG_IMPORT_LIB "${slang_bin_SOURCE_DIR}/lib/slang-compiler.lib")
elseif(UNIX)
    set(SLANG_SHARED_LIB "${slang_bin_SOURCE_DIR}/lib/libslang-compiler.so.0.${SLANG_VERSION}")
endif()

add_library(slang SHARED IMPORTED)
set_target_properties(slang PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${slang_bin_SOURCE_DIR}/include"
    IMPORTED_LOCATION "${SLANG_SHARED_LIB}"
)

if(WIN32)
    set_target_properties(slang PROPERTIES IMPORTED_IMPLIB "${SLANG_IMPORT_LIB}")
endif()

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
# END VMA

# GLM
add_subdirectory(GLM)

target_compile_definitions(vendor INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)
# END GLM

# STB
target_include_directories(vendor INTERFACE stb-image)

target_compile_definitions(vendor INTERFACE STB_IMAGE_IMPLEMENTATION)
# END STB

# GLTF
target_include_directories(vendor INTERFACE tinygltf)
target_compile_definitions(vendor INTERFACE TINYGLTF_IMPLEMENTATION)
# END GLTF

# OBJ
target_include_directories(vendor INTERFACE tinyobj)

target_compile_definitions(vendor INTERFACE
    TINYOBJLOADER_IMPLEMENTATION
    TINYOBJLOADER_USE_MAPBOX_EARCUT
    TINYOBJLOADER_STREAM_READER_MAX_BYTES=-1
    TINYOBJLOADER_USE_MULTITHREADING
    TINYOBJLOADER_USE_SIMD
)

# END OBJ

# SDL3
# cmake -B build -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=0 -DSDL_STATIC=1 -DSDL_TESTS=0 -DSDL_GPU=0 -DSDL_RENDER=0 -DSDL_CAMERA=0 -DSDL_JOYSTICK=0 -DSDL_HAPTIC=0 -DSDL_POWER=0 -DSDL_SENSOR=0 -DSDL_AVX=0 -DSDL_AVX2=0 -DSDL_AVX512F=0 -DSDL_OPENGL=0 -DSDL_OPENGLES=0 -DSDL_DIRECTX=0 -G Ninja
if(WIN32)
    add_library(sdl SHARED IMPORTED)
    set(SDL_SHARED_LIB "${CMAKE_SOURCE_DIR}/vendor/SDL3-bin/bin/SDL3.dll")
    set_target_properties(sdl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/vendor/SDL3-bin/include"
        IMPORTED_LOCATION "${SDL_SHARED_LIB}"
        IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/vendor/SDL3-bin/lib/libSDL3.dll.a"
    )

    add_custom_target(CopySdlLib
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL_SHARED_LIB}" ${CMAKE_BINARY_DIR}
    )
    install(PROGRAMS "${SDL_SHARED_LIB}"
        DESTINATION .
    )
    add_dependencies(vendor CopySdlLib)

elseif(UNIX)
    add_library(sdl STATIC IMPORTED)
    set_target_properties(sdl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/vendor/SDL3-bin/include"
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/SDL3-bin/lib/libSDL3.a"
    )
endif()
# END SDL3

# IMGUI

# END IMGUI

target_link_libraries(vendor INTERFACE
    sdl
    slang
    VulkanMemoryAllocator-Hpp::VulkanMemoryAllocator-Hpp
    glm::glm
    Console
    Timer
)
