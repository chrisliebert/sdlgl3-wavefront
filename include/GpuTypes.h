#ifndef _GPU_TYPES_H_
#define _GPU_TYPES_H_

#include "Common.h"

// Uniform buffer structure for our shaders (used as push constants)
struct Uniforms {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

#endif // _GPU_TYPES_H_