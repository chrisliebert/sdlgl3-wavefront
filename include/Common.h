#ifndef _COMMON_H_
#define _COMMON_H_

#define GLEW_STATIC 1
#define _DEBUG 0

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tiny_obj_loader.h"

#define CACHE_DIRECTORY "cache"
#define CONFIG_DIRECTORY "config"
#define MODEL_DIRECTORY "models"
#define TEXTURE_DIRECTORY "textures"
#define SHADER_DIRECTORY "shaders"

// Cross-platform directory separator
#ifdef _WIN32
    #define DIRECTORY_SEPARATOR "\\"
#else
    #define DIRECTORY_SEPARATOR "/"
#endif

// MSVC uses strcpy_s instead of strcpy
#ifdef _MSC_VER
#define strcpy(A, B) strcpy_s(A, B)
#endif

typedef struct {
    GLfloat vertex[3];
    GLfloat normal[3];
    GLfloat textureCoordinate[2];
} Vertex;

#endif
