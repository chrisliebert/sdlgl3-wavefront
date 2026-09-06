cmake_minimum_required(VERSION 3.28)

project(sdlglapp LANGUAGES CXX VERSION 1.0.0)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Compiler-specific settings
if(MSVC)
    add_compile_options(/W4 /FS)
    # Disable specific warnings for third-party code
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

# FetchContent settings - keep artifacts inside build directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "FetchContent base directory" FORCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED OFF CACHE BOOL "Allow FetchContent to update" FORCE)

include(FetchContent)

# --- Options ---
option(BUILD_VULKAN_BACKEND "Enable the Vulkan rendering backend" ON)

# --- Dependencies ---

# Vulkan
if(BUILD_VULKAN_BACKEND)
    find_package(Vulkan OPTIONAL)
    if(Vulkan_FOUND)
        message(STATUS "Vulkan found, enabling Vulkan backend.")
        add_compile_definitions(HAVE_VULKAN)
    else()
        message(WARNING "Vulkan SDK not found, disabling Vulkan backend. Install the Vulkan SDK to enable it.")
        set(BUILD_VULKAN_BACKEND OFF)
    endif()
endif()

# Vulkan-Hpp (Header-only C++ bindings)
if(BUILD_VULKAN_BACKEND AND Vulkan_FOUND)
    FetchContent_Declare(
        vulkan-hpp
        URL https://raw.githubusercontent.com/KhronosGroup/Vulkan-Hpp/main/vulkan/vulkan.hpp
        DOWNLOAD_NO_EXTRACT TRUE
    )
    FetchContent_MakeAvailable(vulkan-hpp)
    # The file is at ${vulkan-hpp_SOURCE_DIR}/vulkan.hpp
    # The directory to include is just ${vulkan-hpp_SOURCE_DIR}
    set(VULKAN_HPP_INCLUDE_DIR ${vulkan-hpp_SOURCE_DIR})
endif()

# GLM (Header-only math library)
set(GLM_SUBMODULE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glm")
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Disable GLM library build for header-only usage" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "Disable GLM tests" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "Disable GLM install rules" FORCE)
if(EXISTS "${GLM_SUBMODULE_ROOT}/CMakeLists.txt")
    add_subdirectory("${GLM_SUBMODULE_ROOT}" glm)
else()
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
    )
    FetchContent_MakeAvailable(glm)
endif()

# Glad (GL loader) - prefer the submodule's generated target, fall back to a local interface target.
set(GLAD_SUBMODULE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glad")
set(GLAD_LINK_TARGET "")
if(EXISTS "${GLAD_SUBMODULE_ROOT}/CMakeLists.txt")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
    add_subdirectory("${GLAD_SUBMODULE_ROOT}" glad-submodule)
    if(TARGET glad)
        set(GLAD_LINK_TARGET glad)
    endif()
endif()

if(NOT GLAD_LINK_TARGET)
    add_library(glad_lib INTERFACE)
    if(EXISTS "${GLAD_SUBMODULE_ROOT}/src/glad.c" AND EXISTS "${GLAD_SUBMODULE_ROOT}/include/glad/glad.h")
        target_sources(glad_lib INTERFACE "${GLAD_SUBMODULE_ROOT}/src/glad.c")
        target_include_directories(glad_lib INTERFACE "${GLAD_SUBMODULE_ROOT}/include")
    else()
        target_sources(glad_lib INTERFACE dependencies/glad/src/glad.c)
        target_include_directories(glad_lib INTERFACE dependencies/glad/include)
    endif()
    set(GLAD_LINK_TARGET glad_lib)
endif()

# --- Application ---

set(APP_SOURCES
    src/Cache.cpp
    src/Camera.cpp
    src/ConfigLoader.cpp
    src/Frustum.cpp
    src/GpuProgram.cpp
    src/main.cpp
    src/OpenGLBackend.cpp
    src/Renderer.cpp
    src/SceneNode.cpp
    src/Shader.cpp
    src/PathUtil.cpp
    src/MathUtil.cpp
    src/RenderBackendFactory.cpp
    src/SdlGpuBackend.cpp
)

add_executable(sdlglapp ${APP_SOURCES})

target_compile_features(sdlglapp PRIVATE cxx_std_20)
target_compile_definitions(sdlglapp PRIVATE 
    SDL_ENABLE_OLD_NAMES=1 
    GLM_ENABLE_EXPERIMENTAL=1
)

target_include_directories(sdlglapp PRIVATE include)
target_include_directories(sdlglapp PRIVATE dependencies)
target_include_directories(sdlglapp PRIVATE "${GLM_SUBMODULE_ROOT}")

if(BUILD_VULKAN_BACKEND AND Vulkan_FOUND)
    target_include_directories(sdlglapp PRIVATE "${VULKAN_HPP_INCLUDE_DIR}")
endif()

# --- Dependency Resolution ---
if(BUILD_VULKAN_BACKEND AND Vulkan_FOUND)
    list(APPEND SDL_LINK_LIBS Vulkan::Vulkan)
endif()

set(SDL3_CORE_FOUND FALSE)
set(SDL3_IMAGE_FOUND FALSE)
set(FETCHCONTENT_MAKEAVAILABLE_LIST)

# Force SDL3_image to use non-vendored dependencies on this workspace.
# The vendored zlib subfolder is not present, so leaving this at the MSVC default
# causes configuration to fail before the real project even starts.
set(SDLIMAGE_VENDORED OFF CACHE BOOL "Use system/native backends for SDL3_image" FORCE)
set(SDLIMAGE_BACKEND_WIC OFF CACHE BOOL "Disable WIC backend" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "Enable stb backend" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "Disable AVIF" FORCE)
set(SDLIMAGE_AVIF_SAVE OFF CACHE BOOL "Disable AVIF save" FORCE)

# 1. Try CMake package config or an initialized submodule checkout
set(SDL3_SUBMODULE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/sdl3")
if(EXISTS "${SDL3_SUBMODULE_ROOT}/CMakeLists.txt")
    add_subdirectory("${SDL3_SUBMODULE_ROOT}" sdl3-submodule)
    if(TARGET SDL3::SDL3)
        set(SDL3_CORE_FOUND TRUE)
        list(APPEND SDL_LINK_LIBS SDL3::SDL3)
    endif()
endif()

find_package(SDL3 3.4.12 CONFIG QUIET)
if(TARGET SDL3::SDL3)
    set(SDL3_CORE_FOUND TRUE)
    list(APPEND SDL_LINK_LIBS SDL3::SDL3)
endif()

set(SDL3_IMAGE_SUBMODULE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/sdl3_image")
if(EXISTS "${SDL3_IMAGE_SUBMODULE_ROOT}/CMakeLists.txt")
    add_subdirectory("${SDL3_IMAGE_SUBMODULE_ROOT}" sdl3-image-submodule)
    if(TARGET SDL3_image::SDL3_image)
        set(SDL3_IMAGE_FOUND TRUE)
        list(APPEND SDL_LINK_LIBS SDL3_image::SDL3_image)
    endif()
endif()

find_package(SDL3_image CONFIG QUIET)
if(TARGET SDL3_image::SDL3_image)
    set(SDL3_IMAGE_FOUND TRUE)
    list(APPEND SDL_LINK_LIBS SDL3_image::SDL3_image)
endif()

# 2. Fallback: pkg-config for non-Windows
if(NOT WIN32)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        if(NOT SDL3_CORE_FOUND)
            pkg_search_module(SDL3_PC QUIET sdl3)
            if(SDL3_PC_FOUND)
                set(SDL3_CORE_FOUND TRUE)
                list(APPEND SDL_LINK_LIBS ${SDL3_PC_LIBRARIES})
                target_include_directories(sdlglapp PRIVATE ${SDL3_PC_INCLUDE_DIRS})
                target_link_directories(sdlglapp PRIVATE ${SDL3_PC_LIBRARY_DIRS})
            endif()
        endif()
        if(NOT SDL3_IMAGE_FOUND)
            pkg_search_module(SDL3_IMAGE_PC QUIET SDL3_image sdl3-image)
            if(SDL3_IMAGE_PC_FOUND)
                set(SDL3_IMAGE_FOUND TRUE)
                list(APPEND SDL_LINK_LIBS ${SDL3_IMAGE_PC_LIBRARIES})
                target_include_directories(sdlglapp PRIVATE ${SDL3_IMAGE_PC_INCLUDE_DIRS})
                target_link_directories(sdlglapp PRIVATE ${SDL3_IMAGE_PC_LIBRARY_DIRS})
            endif()
        endif()
    endif()
endif()

# 3. Auto-fetch if not found and option is enabled
option(SDL3_FETCH_IF_MISSING "Fetch SDL3/SDL3_image automatically when not found" ON)
if(SDL3_FETCH_IF_MISSING)
    if(NOT SDL3_CORE_FOUND)
        if(EXISTS "${SDL3_SUBMODULE_ROOT}/CMakeLists.txt")
            add_subdirectory("${SDL3_SUBMODULE_ROOT}" sdl3-submodule)
        else()
            FetchContent_Declare(
                SDL3
                GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
                GIT_TAG release-3.4.14
                GIT_SHALLOW TRUE
            )
            list(APPEND FETCHCONTENT_MAKEAVAILABLE_LIST SDL3)
        endif()
    endif()

    if(NOT SDL3_IMAGE_FOUND)
        if(EXISTS "${SDL3_IMAGE_SUBMODULE_ROOT}/CMakeLists.txt")
            add_subdirectory("${SDL3_IMAGE_SUBMODULE_ROOT}" sdl3-image-submodule)
        else()
            FetchContent_Declare(
                SDL3_image
                GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
                GIT_TAG release-3.4.4
                GIT_SHALLOW TRUE
                GIT_SUBMODULES ""
                GIT_SUBMODULES_RECURSE FALSE
            )
            list(APPEND FETCHCONTENT_MAKEAVAILABLE_LIST SDL3_image)
        endif()
    endif()

    if(FETCHCONTENT_MAKEAVAILABLE_LIST)
        FetchContent_MakeAvailable(${FETCHCONTENT_MAKEAVAILABLE_LIST})
    endif()

    if(NOT SDL3_CORE_FOUND AND TARGET SDL3::SDL3)
        set(SDL3_CORE_FOUND TRUE)
        list(APPEND SDL_LINK_LIBS SDL3::SDL3)
    endif()
    if(NOT SDL3_IMAGE_FOUND AND TARGET SDL3_image::SDL3_image)
        set(SDL3_IMAGE_FOUND TRUE)
        list(APPEND SDL_LINK_LIBS SDL3_image::SDL3_image)
    endif()
endif()

# Final checks
if(NOT SDL3_CORE_FOUND)
    message(FATAL_ERROR "SDL3 not found. Install it or use -DSDL3_FETCH_IF_MISSING=ON")
endif()
if(NOT SDL3_IMAGE_FOUND)
    message(FATAL_ERROR "SDL3_image not found. Install it or use -DSDL3_FETCH_IF_MISSING=ON")
endif()

# OpenGL
find_package(OpenGL REQUIRED)

add_library(imgui_lib STATIC
    dependencies/imgui/imgui.cpp
    dependencies/imgui/imgui_draw.cpp
    dependencies/imgui/imgui_tables.cpp
    dependencies/imgui/imgui_widgets.cpp
    dependencies/imgui/backends/imgui_impl_sdl3.cpp
    dependencies/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_lib PUBLIC
    dependencies/imgui
    dependencies/imgui/backends
)



target_link_libraries(imgui_lib PUBLIC
    ${SDL_LINK_LIBS}
    OpenGL::GL
)

# Link all libraries
target_link_libraries(sdlglapp PRIVATE
    ${GLAD_LINK_TARGET}
    OpenGL::GL
    ${SDL_LINK_LIBS}
    imgui_lib
)

# Copy models and configs to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/models DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/cache)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cache)

# --- Unit Tests ---
add_subdirectory(tests)
