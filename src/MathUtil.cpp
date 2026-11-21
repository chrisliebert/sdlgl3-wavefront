#include "MathUtil.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <algorithm>
#include <random>
#include <cmath>
#include <vector>

namespace Math {

namespace {

inline bool contains(const Sphere& s, const glm::vec3& p) {
    const float distSq = glm::dot(p - s.center, p - s.center);
    const float tol = std::max(1e-5f, s.radius * 1e-5f);
    const float rTol = s.radius + tol;
    return distSq <= rTol * rTol;
}

inline Sphere sphereFrom0() {
    return Sphere{glm::vec3(0.0f), 0.0f};
}

inline Sphere sphereFrom1(const glm::vec3& p) {
    return Sphere{p, 0.0f};
}

inline Sphere sphereFrom2(const glm::vec3& a, const glm::vec3& b) {
    const glm::vec3 center = (a + b) * 0.5f;
    const float radius = glm::distance(a, b) * 0.5f;
    return Sphere{center, radius};
}

Sphere sphereFrom3(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 v1 = a - c;
    const glm::vec3 v2 = b - c;
    const glm::vec3 n = glm::cross(v1, v2);
    const float nLenSq = glm::dot(n, n);

    // If points are collinear or degenerate
    if (nLenSq < 1e-12f) {
        const float d_ab = glm::dot(a - b, a - b);
        const float d_bc = glm::dot(b - c, b - c);
        const float d_ca = glm::dot(c - a, c - a);
        if (d_ab >= d_bc && d_ab >= d_ca) return sphereFrom2(a, b);
        if (d_bc >= d_ab && d_bc >= d_ca) return sphereFrom2(b, c);
        return sphereFrom2(c, a);
    }

    // Circumcenter of triangle in 3D
    const glm::vec3 offset = glm::cross(glm::dot(v1, v1) * v2 - glm::dot(v2, v2) * v1, n) / (2.0f * nLenSq);
    const glm::vec3 center = c + offset;
    const float radius = glm::length(offset);
    Sphere s{center, radius};

    // Check if any 2-point sphere encloses all 3 points with smaller radius (e.g. obtuse triangle)
    const Sphere s_ab = sphereFrom2(a, b);
    if (s_ab.radius < s.radius && contains(s_ab, c)) s = s_ab;
    const Sphere s_bc = sphereFrom2(b, c);
    if (s_bc.radius < s.radius && contains(s_bc, a)) s = s_bc;
    const Sphere s_ca = sphereFrom2(c, a);
    if (s_ca.radius < s.radius && contains(s_ca, b)) s = s_ca;

    return s;
}

Sphere sphereFrom4(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4) {
    const glm::vec3 v1 = p1 - p4;
    const glm::vec3 v2 = p2 - p4;
    const glm::vec3 v3 = p3 - p4;

    const glm::vec3 c23 = glm::cross(v2, v3);
    const glm::vec3 c31 = glm::cross(v3, v1);
    const glm::vec3 c12 = glm::cross(v1, v2);

    const float det = glm::dot(v1, c23);

    Sphere bestSphere{glm::vec3(0.0f), std::numeric_limits<float>::infinity()};

    // Robust numerical determinant check: if non-degenerate, compute 4-point circumsphere
    if (std::abs(det) >= 1e-10f) {
        const glm::vec3 offset = (glm::dot(v1, v1) * c23 +
                                  glm::dot(v2, v2) * c31 +
                                  glm::dot(v3, v3) * c12) / (2.0f * det);
        if (!std::isnan(offset.x) && !std::isnan(offset.y) && !std::isnan(offset.z) &&
            !std::isinf(offset.x) && !std::isinf(offset.y) && !std::isinf(offset.z)) {
            bestSphere = Sphere{p4 + offset, glm::length(offset)};
        }
    }

    // Check all 4 subsets of 3 points
    const glm::vec3* pts[4] = { &p1, &p2, &p3, &p4 };
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            for (int k = j + 1; k < 4; ++k) {
                Sphere s = sphereFrom3(*pts[i], *pts[j], *pts[k]);
                if (s.radius < bestSphere.radius &&
                    contains(s, p1) && contains(s, p2) &&
                    contains(s, p3) && contains(s, p4)) {
                    bestSphere = s;
                }
            }
        }
    }

    // If still infinite (e.g. collinear), check all 6 pairs of 2 points
    if (std::isinf(bestSphere.radius)) {
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                Sphere s = sphereFrom2(*pts[i], *pts[j]);
                if (s.radius < bestSphere.radius &&
                    contains(s, p1) && contains(s, p2) &&
                    contains(s, p3) && contains(s, p4)) {
                    bestSphere = s;
                }
            }
        }
    }

    return bestSphere;
}

Sphere welzl3(const std::vector<glm::vec3>& P, size_t n, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
    Sphere s = sphereFrom3(p1, p2, p3);
    for (size_t i = 0; i < n; ++i) {
        if (!contains(s, P[i])) {
            s = sphereFrom4(p1, p2, p3, P[i]);
        }
    }
    return s;
}

Sphere welzl2(const std::vector<glm::vec3>& P, size_t n, const glm::vec3& p1, const glm::vec3& p2) {
    Sphere s = sphereFrom2(p1, p2);
    for (size_t i = 0; i < n; ++i) {
        if (!contains(s, P[i])) {
            s = welzl3(P, i, p1, p2, P[i]);
        }
    }
    return s;
}

Sphere welzl1(const std::vector<glm::vec3>& P, size_t n, const glm::vec3& p1) {
    Sphere s = sphereFrom1(p1);
    for (size_t i = 0; i < n; ++i) {
        if (!contains(s, P[i])) {
            s = welzl2(P, i, p1, P[i]);
        }
    }
    return s;
}

Sphere welzl(std::vector<glm::vec3>& P) {
    if (P.empty()) return sphereFrom0();
    if (P.size() == 1) return sphereFrom1(P[0]);

    // Randomize point order for expected linear time
    std::mt19937 rng(1337);
    std::shuffle(P.begin(), P.end(), rng);

    Sphere s = sphereFrom2(P[0], P[1]);
    for (size_t i = 2; i < P.size(); ++i) {
        if (!contains(s, P[i])) {
            s = welzl1(P, i, P[i]);
        }
    }
    return s;
}

} // anonymous namespace

Sphere calculateBoundingSphere(const Vertex* vertices, size_t numVertices) {
    if (numVertices == 0) return sphereFrom0();
    if (numVertices == 1) return sphereFrom1(glm::vec3(vertices[0].vertex[0], vertices[0].vertex[1], vertices[0].vertex[2]));
    if (numVertices == 2) {
        const glm::vec3 p0(vertices[0].vertex[0], vertices[0].vertex[1], vertices[0].vertex[2]);
        const glm::vec3 p1(vertices[1].vertex[0], vertices[1].vertex[1], vertices[1].vertex[2]);
        return sphereFrom2(p0, p1);
    }

    std::vector<glm::vec3> pts;
    pts.reserve(numVertices);
    for (size_t i = 0; i < numVertices; ++i) {
        pts.emplace_back(vertices[i].vertex[0], vertices[i].vertex[1], vertices[i].vertex[2]);
    }
    Sphere s = welzl(pts);

    // Safety pass ensuring exact enclosing boundary despite any float rounding
    float maxDistSq = 0.0f;
    for (size_t i = 0; i < numVertices; ++i) {
        const glm::vec3 p(vertices[i].vertex[0], vertices[i].vertex[1], vertices[i].vertex[2]);
        const float dSq = glm::dot(p - s.center, p - s.center);
        if (dSq > maxDistSq) {
            maxDistSq = dSq;
        }
    }
    const float actualRadius = std::sqrt(maxDistSq);
    if (actualRadius > s.radius) {
        s.radius = actualRadius;
    }
    return s;
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

} // namespace Math
