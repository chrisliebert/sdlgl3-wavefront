#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <cmath>
#include <glm/glm.hpp>

static constexpr float TEST_EPSILON = 1e-4f;

static bool approxEqual(float a, float b, float epsilon = TEST_EPSILON) {
    return std::fabs(a - b) < epsilon;
}

static bool vec3ApproxEqual(glm::vec3 a, glm::vec3 b, float epsilon = TEST_EPSILON) {
    return approxEqual(a.x, b.x, epsilon) &&
           approxEqual(a.y, b.y, epsilon) &&
           approxEqual(a.z, b.z, epsilon);
}

#endif // TEST_HELPERS_H
