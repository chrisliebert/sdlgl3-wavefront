#ifndef _COMMON_H_
#define _COMMON_H_

#include <algorithm>
#include <cstdlib>
#include <cstdio>
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

#include <SDL.h>
#include "glad/glad.h"
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

#define MAX_MATERIAL_NAME_STRING_LENGTH 256
#define MAX_NODE_NAME_STRING_LENGTH 256

typedef struct {
    GLfloat vertex[3];
    GLfloat normal[3];
    GLfloat textureCoordinate[2];
} Vertex;

#endif
