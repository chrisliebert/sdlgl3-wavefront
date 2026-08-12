#ifndef _MATH_UTIL_H_
#define _MATH_UTIL_H_

#include "Common.h"
#include <vector>

namespace Math {
    struct Sphere {
        glm::vec3 center;
        float radius;
    };

    Sphere calculateBoundingSphere(const Vertex* vertices, size_t numVertices);
}

#endif // _MATH_UTIL_H_
