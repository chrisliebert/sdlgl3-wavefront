#ifndef _COMMON_H_
#define _COMMON_H_

#define _DEBUG 0
#define GLEW_STATIC 1

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include <stdlib.h>
#include <stdio.h>

#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/matrix.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tiny_obj_loader.h"

#define MODEL_DIRECTORY "models"
#define TEXTURE_DIRECTORY "textures"

// Cross-platform directory separator
#ifdef _WIN32
    #define DIRECTORY_SEPARATOR "\\"
#else
    #define DIRECTORY_SEPARATOR "/"
#endif

typedef struct {
    GLfloat vertex[3];
    GLfloat normal[3];
    GLfloat textureCoordinate[2];
} Vertex;

#endif
