#ifndef _COMMON_H_
#define _COMMON_H_

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#if __has_include(<SDL3/SDL.h>)
#include <SDL3/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL headers not found"
#endif
#include "glad/glad.h"

// ============================================================================
// Math constants and helpers
// ============================================================================
namespace Math
{
    inline constexpr glm::vec3 getLightPositionOffset() { return glm::vec3(10.0f, 50.0f, 0.0f); }
}

// ============================================================================
// Directory and path constants (inline constexpr for C++17+)
// ============================================================================
inline constexpr const char* CACHE_DIRECTORY_STR = "cache";
inline constexpr const char* CONFIG_DIRECTORY_STR = "config";
inline constexpr const char* MODEL_DIRECTORY_STR = "models";
inline constexpr const char* TEXTURE_DIRECTORY_STR = "textures";
inline constexpr const char* SHADER_DIRECTORY_STR = "shaders";

// Cross-platform directory separator
#ifdef _WIN32
    inline constexpr const char* DIRECTORY_SEPARATOR_STR = "\\";
#else
    inline constexpr const char* DIRECTORY_SEPARATOR_STR = "/";
#endif

// Legacy macro aliases (deprecated; prefer the constexpr constants above)
#define CACHE_DIRECTORY CACHE_DIRECTORY_STR
#define CONFIG_DIRECTORY CONFIG_DIRECTORY_STR
#define MODEL_DIRECTORY MODEL_DIRECTORY_STR
#define TEXTURE_DIRECTORY TEXTURE_DIRECTORY_STR
#define SHADER_DIRECTORY SHADER_DIRECTORY_STR
#define DIRECTORY_SEPARATOR DIRECTORY_SEPARATOR_STR

// ============================================================================
// String length limits
// ============================================================================
inline constexpr std::size_t MAX_MATERIAL_NAME_STRING_LENGTH = 256;
inline constexpr std::size_t MAX_NODE_NAME_STRING_LENGTH = 256;

// ============================================================================
// GL error checking (macro for convenient file/line context)
// ============================================================================
inline void checkForGLErrorHelper(const char* file, int line)
{
    GLenum err = glGetError();
    while (err != GL_NO_ERROR)
    {
        const char* errorStr = nullptr;
        switch (err)
        {
            case GL_INVALID_ENUM:
                errorStr = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorStr = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorStr = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                errorStr = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                errorStr = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                errorStr = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                errorStr = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
        fprintf(stderr, "GL_ERROR %s at %s:%d\n", errorStr, file, line);
        err = glGetError();
    }
}

#define checkForGLError() checkForGLErrorHelper(__FILE__, __LINE__)

// ============================================================================
// Vertex structure (legacy C-style; prefer glm::vec3 in new code)
// ============================================================================
struct Vertex {
    float vertex[3];
    float normal[3];
    float textureCoordinate[2];
};

// ============================================================================
// Frame context (per-frame timing and viewport state)
// ============================================================================
struct FrameContext {
    Uint64 frameStartTicksNs = 0;
    double absoluteTimeMs = 0.0;
    double deltaTimeMs = 0.0;
    int viewportWidth = 0;
    int viewportHeight = 0;
};

// ============================================================================
// RAII wrappers for SDL types
// ============================================================================
struct SdlSurfaceDeleter {
    void operator()(SDL_Surface* surface) const {
        if (surface) {
#if SDL_MAJOR_VERSION >= 3
            SDL_DestroySurface(surface);
#else
            SDL_FreeSurface(surface);
#endif
        }
    }
};

#endif // _COMMON_H_
