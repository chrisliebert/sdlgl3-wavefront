#include "MathUtil.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <limits>
#include <algorithm>

namespace Math {

Sphere calculateBoundingSphere(const Vertex* vertices, size_t numVertices) {
    Sphere sphere;
    if (numVertices == 0) {
        sphere.center = glm::vec3(0.0f);
        sphere.radius = 0.0f;
        return sphere;
    }

    // Find the min and max points along X, Y, and Z axes
    glm::vec3 min_x = glm::vec3(vertices[0].vertex[0], vertices[0].vertex[1], vertices[0].vertex[2]);
    glm::vec3 max_x = min_x;
    glm::vec3 min_y = min_x;
    glm::vec3 max_y = min_x;
    glm::vec3 min_z = min_x;
    glm::vec3 max_z = min_x;

    for (size_t i = 1; i < numVertices; ++i) {
        glm::vec3 p(vertices[i].vertex[0], vertices[i].vertex[1], vertices[i].vertex[2]);
        if (p.x < min_x.x) min_x = p;
        if (p.x > max_x.x) max_x = p;
        if (p.y < min_y.y) min_y = p;
        if (p.y > max_y.y) max_y = p;
        if (p.z < min_z.z) min_z = p;
        if (p.z > max_z.z) max_z = p;
    }

    float dist_x_sq = glm::distance2(max_x, min_x);
    float dist_y_sq = glm::distance2(max_y, min_y);
    float dist_z_sq = glm::distance2(max_z, min_z);

    glm::vec3 p1 = min_x;
    glm::vec3 p2 = max_x;
    if (dist_y_sq > dist_x_sq && dist_y_sq > dist_z_sq) {
        p1 = min_y;
        p2 = max_y;
    } else if (dist_z_sq > dist_x_sq && dist_z_sq > dist_y_sq) {
        p1 = min_z;
        p2 = max_z;
    }
    
    sphere.center = (p1 + p2) * 0.5f;
    sphere.radius = glm::distance(p1, p2) * 0.5f;

    for (size_t i = 0; i < numVertices; ++i) {
        glm::vec3 point(vertices[i].vertex[0], vertices[i].vertex[1], vertices[i].vertex[2]);
        float distSq = glm::distance2(point, sphere.center);
        if (distSq > sphere.radius * sphere.radius) {
            float dist = glm::sqrt(distSq);
            glm::vec3 dir = (point - sphere.center) / dist;
            glm::vec3 p_opposite = sphere.center - sphere.radius * dir;
            sphere.center = (p_opposite + point) * 0.5f;
            sphere.radius = glm::distance(p_opposite, point) * 0.5f;
        }
    }

    return sphere;
}

}
