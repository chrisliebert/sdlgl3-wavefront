#include "MathUtil.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

    const float dist_x_sq = glm::dot(max_x - min_x, max_x - min_x);
    const float dist_y_sq = glm::dot(max_y - min_y, max_y - min_y);
    const float dist_z_sq = glm::dot(max_z - min_z, max_z - min_z);

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
        const float distSq = glm::dot(point - sphere.center, point - sphere.center);
        if (distSq > sphere.radius * sphere.radius) {
            const float dist = std::sqrt(distSq);
            const glm::vec3 dir = (point - sphere.center) / dist;
            const glm::vec3 p_opposite = sphere.center - sphere.radius * dir;
            sphere.center = (p_opposite + point) * 0.5f;
            sphere.radius = glm::length(p_opposite - point) * 0.5f;
        }
    }

    return sphere;
}

Sphere transformBoundingSphere(const glm::mat4& transform, const glm::vec3& center, float radius) {
    const glm::vec3 worldCenter = transform * glm::vec4(center, 1.0f);

    const glm::vec3 xAxis = glm::vec3(transform[0]);
    const glm::vec3 yAxis = glm::vec3(transform[1]);
    const glm::vec3 zAxis = glm::vec3(transform[2]);

    float maxAxisScale = glm::length(xAxis);
    const float yScale = glm::length(yAxis);
    const float zScale = glm::length(zAxis);
    if (yScale > maxAxisScale) maxAxisScale = yScale;
    if (zScale > maxAxisScale) maxAxisScale = zScale;

    return Sphere{worldCenter, radius * std::max(1.0f, maxAxisScale)};
}

}
