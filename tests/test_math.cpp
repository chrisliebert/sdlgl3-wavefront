#include "MathUtil.h"
#include "test_helpers.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

static Vertex makeVertex(float x, float y, float z) {
    Vertex v{};
    v.vertex[0] = x;
    v.vertex[1] = y;
    v.vertex[2] = z;
    return v;
}

static void assertAllVerticesEnclosed(const std::vector<Vertex>& vertices, const Math::Sphere& sphere, float epsilon = 1e-4f) {
    for (size_t i = 0; i < vertices.size(); ++i) {
        glm::vec3 p(vertices[i].vertex[0], vertices[i].vertex[1], vertices[i].vertex[2]);
        float dist = glm::length(p - sphere.center);
        assert(dist <= sphere.radius + epsilon);
    }
}

// ============================================================================
// Welzl / Ritter Bounding Sphere Tests
// ============================================================================

static void testBoundingSphereEmpty() {
    std::cout << "  [TEST] Bounding sphere: 0 vertices (empty)... ";
    Math::Sphere s = Math::calculateBoundingSphere(static_cast<const Vertex*>(nullptr), 0);
    assert(vec3ApproxEqual(s.center, glm::vec3(0.0f)));
    assert(approxEqual(s.radius, 0.0f));
    std::cout << "PASSED" << std::endl;
}

static void testBoundingSphereSingleVertex() {
    std::cout << "  [TEST] Bounding sphere: 1 vertex... ";
    std::vector<Vertex> verts = { makeVertex(3.5f, -2.1f, 7.8f) };
    Math::Sphere s = Math::calculateBoundingSphere(verts.data(), verts.size());
    assert(vec3ApproxEqual(s.center, glm::vec3(3.5f, -2.1f, 7.8f)));
    assert(approxEqual(s.radius, 0.0f));
    assertAllVerticesEnclosed(verts, s);
    std::cout << "PASSED" << std::endl;
}

static void testBoundingSphereTwoVertices() {
    std::cout << "  [TEST] Bounding sphere: 2 vertices (line segment)... ";
    glm::vec3 p1(-2.0f, 1.0f, 4.0f);
    glm::vec3 p2(4.0f, -3.0f, 2.0f);
    std::vector<Vertex> verts = { makeVertex(p1.x, p1.y, p1.z), makeVertex(p2.x, p2.y, p2.z) };
    Math::Sphere s = Math::calculateBoundingSphere(verts.data(), verts.size());

    glm::vec3 expectedCenter = (p1 + p2) * 0.5f;
    float expectedRadius = glm::distance(p1, p2) * 0.5f;
    assert(vec3ApproxEqual(s.center, expectedCenter));
    assert(approxEqual(s.radius, expectedRadius));
    assertAllVerticesEnclosed(verts, s);
    std::cout << "PASSED" << std::endl;
}

static void testBoundingSphereThreeVertices() {
    std::cout << "  [TEST] Bounding sphere: 3 vertices (triangles)... ";

    // Right triangle with hypotenuse centered at origin
    // Vertices at (-1, 0, 0), (1, 0, 0), (0, 1, 0)
    std::vector<Vertex> rightTri = {
        makeVertex(-1.0f, 0.0f, 0.0f),
        makeVertex(1.0f, 0.0f, 0.0f),
        makeVertex(0.0f, 1.0f, 0.0f)
    };
    Math::Sphere sRight = Math::calculateBoundingSphere(rightTri.data(), rightTri.size());
    assert(vec3ApproxEqual(sRight.center, glm::vec3(0.0f, 0.0f, 0.0f)));
    assert(approxEqual(sRight.radius, 1.0f));
    assertAllVerticesEnclosed(rightTri, sRight);

    // Equilateral triangle on unit circle
    const float sqrt3_2 = std::sqrt(3.0f) * 0.5f;
    std::vector<Vertex> eqTri = {
        makeVertex(1.0f, 0.0f, 0.0f),
        makeVertex(-0.5f, sqrt3_2, 0.0f),
        makeVertex(-0.5f, -sqrt3_2, 0.0f)
    };
    Math::Sphere sEq = Math::calculateBoundingSphere(eqTri.data(), eqTri.size());
    assertAllVerticesEnclosed(eqTri, sEq);
    assert(sEq.radius >= 1.0f - 1e-4f);

    std::cout << "PASSED" << std::endl;
}

static void testBoundingSphereTetrahedron() {
    std::cout << "  [TEST] Bounding sphere: 4 vertices (regular tetrahedron)... ";
    // Regular tetrahedron vertices with distance sqrt(3) from origin
    std::vector<Vertex> tetra = {
        makeVertex( 1.0f,  1.0f,  1.0f),
        makeVertex( 1.0f, -1.0f, -1.0f),
        makeVertex(-1.0f,  1.0f, -1.0f),
        makeVertex(-1.0f, -1.0f,  1.0f)
    };
    Math::Sphere s = Math::calculateBoundingSphere(tetra.data(), tetra.size());
    assertAllVerticesEnclosed(tetra, s);
    // Circumscribed radius is sqrt(3) ~ 1.73205
    assert(s.radius >= std::sqrt(3.0f) - 1e-3f);
    std::cout << "PASSED" << std::endl;
}

static void testBoundingSphereCube() {
    std::cout << "  [TEST] Bounding sphere: 8 vertices (cube [-1, 1]^3)... ";
    
    // When opposing extreme corners define the primary span
    std::vector<Vertex> cubeOpposing = {
        makeVertex(-1.0f, -1.0f, -1.0f),
        makeVertex( 1.0f,  1.0f,  1.0f),
        makeVertex(-1.0f, -1.0f,  1.0f),
        makeVertex(-1.0f,  1.0f, -1.0f),
        makeVertex(-1.0f,  1.0f,  1.0f),
        makeVertex( 1.0f, -1.0f, -1.0f),
        makeVertex( 1.0f, -1.0f,  1.0f),
        makeVertex( 1.0f,  1.0f, -1.0f)
    };
    Math::Sphere sOpposing = Math::calculateBoundingSphere(cubeOpposing.data(), cubeOpposing.size());
    assert(vec3ApproxEqual(sOpposing.center, glm::vec3(0.0f, 0.0f, 0.0f)));
    assert(approxEqual(sOpposing.radius, std::sqrt(3.0f)));
    assertAllVerticesEnclosed(cubeOpposing, sOpposing);

    // Any cube permutation guarantees all vertices enclosed and radius >= sqrt(3)
    std::vector<Vertex> cube = {
        makeVertex(-1.0f, -1.0f, -1.0f),
        makeVertex(-1.0f, -1.0f,  1.0f),
        makeVertex(-1.0f,  1.0f, -1.0f),
        makeVertex(-1.0f,  1.0f,  1.0f),
        makeVertex( 1.0f, -1.0f, -1.0f),
        makeVertex( 1.0f, -1.0f,  1.0f),
        makeVertex( 1.0f,  1.0f, -1.0f),
        makeVertex( 1.0f,  1.0f,  1.0f)
    };
    Math::Sphere s = Math::calculateBoundingSphere(cube.data(), cube.size());
    assert(s.radius >= std::sqrt(3.0f) - 1e-4f);
    assertAllVerticesEnclosed(cube, s);

    std::cout << "PASSED" << std::endl;
}

static void testBoundingSphereEnclosureProperty() {
    std::cout << "  [TEST] Bounding sphere: enclosure property on multi-point set... ";
    std::vector<Vertex> points;
    // Generate 25 points inside a box [-5, 5]^3
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            points.push_back(makeVertex(static_cast<float>(x * 2), static_cast<float>(y * 3), static_cast<float>((x + y) * 1.5f)));
        }
    }
    Math::Sphere s = Math::calculateBoundingSphere(points.data(), points.size());
    assertAllVerticesEnclosed(points, s);
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Transform Bounding Sphere Tests
// ============================================================================

static void testTransformBoundingSphere() {
    std::cout << "  [TEST] Transform bounding sphere (uniform & non-uniform scale)... ";

    const glm::vec3 initialCenter(1.0f, 2.0f, 3.0f);
    const float initialRadius = 4.0f;

    // 1. Identity transform
    {
        glm::mat4 identity(1.0f);
        Math::Sphere s = Math::transformBoundingSphere(identity, initialCenter, initialRadius);
        assert(vec3ApproxEqual(s.center, initialCenter));
        assert(approxEqual(s.radius, initialRadius));
    }

    // 2. Pure translation
    {
        glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, -5.0f, 2.0f));
        Math::Sphere s = Math::transformBoundingSphere(trans, initialCenter, initialRadius);
        assert(vec3ApproxEqual(s.center, initialCenter + glm::vec3(10.0f, -5.0f, 2.0f)));
        assert(approxEqual(s.radius, initialRadius));
    }

    // 3. Uniform scaling (> 1)
    {
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(2.5f));
        Math::Sphere s = Math::transformBoundingSphere(scaleMat, initialCenter, initialRadius);
        assert(vec3ApproxEqual(s.center, initialCenter * 2.5f));
        assert(approxEqual(s.radius, initialRadius * 2.5f));
    }

    // 4. Non-uniform scaling (expanding)
    {
        glm::mat4 scaleNonUniform = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 5.0f, 1.5f));
        Math::Sphere s = Math::transformBoundingSphere(scaleNonUniform, initialCenter, initialRadius);
        assert(vec3ApproxEqual(s.center, glm::vec3(2.0f, 10.0f, 4.5f)));
        // maxAxisScale = 5.0f -> radius = 4.0 * 5.0 = 20.0f
        assert(approxEqual(s.radius, 20.0f));
    }

    // 5. Non-uniform scaling (sub-unit scale)
    {
        glm::mat4 scaleSubUnit = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.8f, 0.5f));
        Math::Sphere s = Math::transformBoundingSphere(scaleSubUnit, initialCenter, initialRadius);
        assert(vec3ApproxEqual(s.center, glm::vec3(0.3f, 1.6f, 1.5f)));
        // std::max(1.0f, maxAxisScale=0.8f) -> 1.0f -> radius = 4.0 * 1.0 = 4.0f
        assert(approxEqual(s.radius, 4.0f));
    }

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Test Suite Runners
// ============================================================================

int run_math_tests() {
    std::cout << "\n=== Math & Bounding Sphere Tests ===" << std::endl;
    testBoundingSphereEmpty();
    testBoundingSphereSingleVertex();
    testBoundingSphereTwoVertices();
    testBoundingSphereThreeVertices();
    testBoundingSphereTetrahedron();
    testBoundingSphereCube();
    testBoundingSphereEnclosureProperty();
    testTransformBoundingSphere();
    std::cout << "All Math & Bounding Sphere tests PASSED!" << std::endl;
    return 0;
}

int runMathTests() {
    return run_math_tests();
}
