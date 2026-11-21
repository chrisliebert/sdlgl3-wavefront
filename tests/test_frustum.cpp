#include "Frustum.h"
#include "MathUtil.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <cassert>

// Tolerance for floating point comparisons
static constexpr float EPSILON = 1e-4f;

static bool approxEqual(float a, float b) {
    return std::fabs(a - b) < EPSILON;
}

// ============================================================================
// Frustum plane extraction tests
// ============================================================================

void testPlaneExtraction() {
    std::cout << "  [TEST] Plane extraction from view-projection matrix... ";
    
    Frustum frustum;
    
    // Create a simple perspective projection matrix
    float fov = 45.0f * 3.14159265f / 180.0f; // 45 degrees in radians
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    glm::mat4 projection = glm::perspective(fov, aspect, nearPlane, farPlane);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, -5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    glm::mat4 vp = projection * view;
    frustum.extractFrustum(vp);
    
    // Verify planes are normalized by testing culling behavior:
    // A point at the center of the frustum should be inside
    // This indirectly verifies planes are correctly extracted and normalized
    assert(frustum.pointInFrustum(0.0f, 0.0f, -4.0f) == true);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Point in frustum tests
// ============================================================================

void testPointInFrustum() {
    std::cout << "  [TEST] Point in frustum culling... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Point at origin should be inside (camera looks at origin from z=10)
    assert(frustum.pointInFrustum(0.0f, 0.0f, 0.0f) == true);
    
    // Point far behind camera should be outside
    assert(frustum.pointInFrustum(0.0f, 0.0f, 20.0f) == false);
    
    // Point far to the left should be outside
    assert(frustum.pointInFrustum(-100.0f, 0.0f, 5.0f) == false);
    
    // Point far to the right should be outside
    assert(frustum.pointInFrustum(100.0f, 0.0f, 5.0f) == false);
    
    // Point above frustum should be outside
    assert(frustum.pointInFrustum(0.0f, 100.0f, 5.0f) == false);
    
    // Point below frustum should be outside
    assert(frustum.pointInFrustum(0.0f, -100.0f, 5.0f) == false);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Sphere in frustum tests
// ============================================================================

void testSphereInFrustum() {
    std::cout << "  [TEST] Sphere in frustum culling... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Small sphere at origin should be inside
    assert(frustum.sphereInFrustum(0.0f, 0.0f, 0.0f, 0.1f) == true);
    
    // Large sphere at origin may extend outside
    // Point with large radius behind camera
    assert(frustum.sphereInFrustum(0.0f, 0.0f, 20.0f, 1.0f) == false);
    
    std::cout << "PASSED" << std::endl;
}

void testSpherePartiallyInFrustum() {
    std::cout << "  [TEST] Sphere partially in frustum... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Fully inside sphere (small, at center)
    int result = frustum.spherePartiallyInFrustum(0.0f, 0.0f, 5.0f, 0.1f);
    assert(result == 2); // fully inside
    
    // Completely outside sphere
    result = frustum.spherePartiallyInFrustum(0.0f, 0.0f, 100.0f, 1.0f);
    assert(result == 0); // completely outside
    
    std::cout << "PASSED" << std::endl;
}

void testSphereInFrustumDistance() {
    std::cout << "  [TEST] Sphere in frustum distance... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Sphere fully inside should return positive distance
    float dist = frustum.sphereInFrustumDistance(0.0f, 0.0f, 5.0f, 0.1f);
    assert(dist > 0.0f);
    
    // Sphere completely outside should return 0
    dist = frustum.sphereInFrustumDistance(0.0f, 0.0f, 100.0f, 1.0f);
    assert(dist == 0.0f);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Cube in frustum tests
// ============================================================================

void testCubeInFrustum() {
    std::cout << "  [TEST] Cube in frustum culling... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Small cube at center should be inside
    assert(frustum.cubeInFrustum(0.0f, 0.0f, 5.0f, 0.1f) == true);
    
    // Large cube far away should be outside
    assert(frustum.cubeInFrustum(0.0f, 0.0f, 100.0f, 10.0f) == false);
    
    std::cout << "PASSED" << std::endl;
}

void testCubePartiallyInFrustum() {
    std::cout << "  [TEST] Cube partially in frustum... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Fully inside cube
    int result = frustum.cubePartiallyInFrustum(0.0f, 0.0f, 5.0f, 0.1f);
    assert(result == 2); // fully inside
    
    // Completely outside cube
    result = frustum.cubePartiallyInFrustum(0.0f, 0.0f, 100.0f, 10.0f);
    assert(result == 0); // completely outside
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Polygon in frustum tests
// ============================================================================

void testPolygonInFrustum() {
    std::cout << "  [TEST] Polygon in frustum culling... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Small triangle at center should be inside
    Point triangle[3] = {
        {0.0f, 0.1f, 5.0f},
        {-0.1f, -0.1f, 5.0f},
        {0.1f, -0.1f, 5.0f}
    };
    assert(frustum.polygonInFrustum(3, triangle) == true);
    
    // Triangle completely outside
    Point outside[3] = {
        {0.0f, 0.0f, 100.0f},
        {1.0f, 0.0f, 100.0f},
        {-1.0f, 0.0f, 100.0f}
    };
    assert(frustum.polygonInFrustum(3, outside) == false);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Edge case tests
// ============================================================================

void testEmptyFrustum() {
    std::cout << "  [TEST] Empty frustum default state... ";
    
    Frustum frustum;
    
    // Default frustum has zero-initialized planes
    // All points should be outside (planes are all zeros, so distance = 0 <= 0)
    assert(frustum.pointInFrustum(0.0f, 0.0f, 0.0f) == false);
    
    std::cout << "PASSED" << std::endl;
}

void testTransformedBoundingSphere() {
    std::cout << "  [TEST] World-space transformed bounding sphere... ";

    const glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));
    const auto worldSphere = Math::transformBoundingSphere(transform, glm::vec3(0.0f, 0.0f, 0.0f), 2.0f);

    assert(std::fabs(worldSphere.center.x - 10.0f) < EPSILON);
    assert(std::fabs(worldSphere.center.y) < EPSILON);
    assert(std::fabs(worldSphere.center.z) < EPSILON);
    assert(std::fabs(worldSphere.radius - 2.0f) < EPSILON);

    std::cout << "PASSED" << std::endl;
}

void testNearPlaneBoundary() {
    std::cout << "  [TEST] Near plane boundary... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Point at near plane boundary (z = 10 - 0.1 = 9.9 in world space)
    // This is approximately at the edge of visibility
    float dist = frustum.sphereInFrustumDistance(0.0f, 0.0f, 9.9f, 0.0f);
    assert(dist >= 0.0f); // Should not be negative
    
    std::cout << "PASSED" << std::endl;
}

void testFarPlaneBoundary() {
    std::cout << "  [TEST] Far plane boundary... ";
    
    Frustum frustum;
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    frustum.extractFrustum(projection * view);
    
    // Point far beyond far plane should be outside
    assert(frustum.pointInFrustum(0.0f, 0.0f, -995.0f) == false);
    
    std::cout << "PASSED" << std::endl;
}

void testNegativeSphereRadius() {
    std::cout << "  [TEST] Negative sphere radius handling... ";
    
    Frustum frustum;
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    frustum.extractFrustum(projection * view);

    // Center deep inside frustum: distance to all planes is > 1.0f
    // With negative radius -0.5f, test is dist <= -(-0.5f) = 0.5f
    // Since dist > 0.5f, sphereInFrustum returns true
    assert(frustum.sphereInFrustum(0.0f, 0.0f, 5.0f, -0.5f) == true);

    // Center very close to near plane (near plane is at z=9.9)
    // At z=9.85, distance to near plane is ~0.05f < 0.5f, so dist <= -radius (0.5f) triggers false
    assert(frustum.sphereInFrustum(0.0f, 0.0f, 9.85f, -0.5f) == false);

    // Sphere distance with negative radius should return 0 if rejected
    assert(frustum.sphereInFrustumDistance(0.0f, 0.0f, 9.85f, -0.5f) == 0.0f);

    std::cout << "PASSED" << std::endl;
}

void testSphereOutsideNearFarPlanes() {
    std::cout << "  [TEST] Sphere center outside near/far planes... ";

    Frustum frustum;
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    frustum.extractFrustum(projection * view);

    // Behind camera (camera eye at z=10, near plane at z=9.9)
    // Sphere center at z=15 (5 units behind camera)
    // Small sphere (radius 1.0) is completely behind near plane -> false
    assert(frustum.sphereInFrustum(0.0f, 0.0f, 15.0f, 1.0f) == false);
    assert(frustum.spherePartiallyInFrustum(0.0f, 0.0f, 15.0f, 1.0f) == 0);

    // Large sphere (radius 10.0) crosses near plane into view volume -> true
    assert(frustum.sphereInFrustum(0.0f, 0.0f, 15.0f, 10.0f) == true);
    assert(frustum.spherePartiallyInFrustum(0.0f, 0.0f, 15.0f, 10.0f) == 1);

    // Beyond far plane (far plane at z=-990.0)
    // Sphere center at z=-1050.0
    // Small sphere (radius 10.0) does not reach far plane -> false
    assert(frustum.sphereInFrustum(0.0f, 0.0f, -1050.0f, 10.0f) == false);
    assert(frustum.spherePartiallyInFrustum(0.0f, 0.0f, -1050.0f, 10.0f) == 0);

    // Large sphere (radius 100.0) crosses far plane into view volume -> true
    assert(frustum.sphereInFrustum(0.0f, 0.0f, -1050.0f, 100.0f) == true);
    assert(frustum.spherePartiallyInFrustum(0.0f, 0.0f, -1050.0f, 100.0f) == 1);

    std::cout << "PASSED" << std::endl;
}

void testDegeneratePointCulling() {
    std::cout << "  [TEST] Degenerate point & polygon culling... ";

    Frustum frustum;
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    frustum.extractFrustum(projection * view);

    // Large / extreme coordinates
    assert(frustum.pointInFrustum(1e7f, 1e7f, 1e7f) == false);
    assert(frustum.pointInFrustum(-1e7f, -1e7f, -1e7f) == false);
    assert(frustum.pointInFrustum(0.0f, 0.0f, 1e7f) == false);

    // Degenerate polygon tests:
    // 0 vertices -> should return false
    assert(frustum.polygonInFrustum(0, nullptr) == false);

    // 1 vertex inside frustum -> true
    Point insidePt = {0.0f, 0.0f, 5.0f};
    assert(frustum.polygonInFrustum(1, &insidePt) == true);

    // 1 vertex outside frustum -> false
    Point outsidePt = {0.0f, 0.0f, 50.0f};
    assert(frustum.polygonInFrustum(1, &outsidePt) == false);

    // 3 identical vertices (degenerate collapsed triangle) inside frustum -> true
    Point degenerateTri[3] = { insidePt, insidePt, insidePt };
    assert(frustum.polygonInFrustum(3, degenerateTri) == true);

    std::cout << "PASSED" << std::endl;
}

void testPointsOnFrustumPlanes() {
    std::cout << "  [TEST] Points on frustum plane boundaries... ";

    Frustum frustum;
    float fov = 45.0f * 3.14159265f / 180.0f;
    glm::mat4 projection = glm::perspective(fov, 1.0f, 0.1f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    frustum.extractFrustum(projection * view);

    // The near plane is located at z = 10.0 - 0.1 = 9.9 in world space
    // Plane equation tests: dist <= 0.0f returns false (outside/on-boundary)
    // Points slightly outside near plane (z > 9.9)
    assert(frustum.pointInFrustum(0.0f, 0.0f, 9.95f) == false);

    // Points slightly inside near plane (z < 9.9)
    assert(frustum.pointInFrustum(0.0f, 0.0f, 9.8f) == true);

    // Points slightly outside far plane (far plane at z = 10.0 - 1000.0 = -990.0)
    assert(frustum.pointInFrustum(0.0f, 0.0f, -995.0f) == false);

    // Points slightly inside far plane
    assert(frustum.pointInFrustum(0.0f, 0.0f, -985.0f) == true);

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Test runner
// ============================================================================

int runFrustumTests() {
    std::cout << "\n=== Frustum Tests ===" << std::endl;
    
    testPlaneExtraction();
    testPointInFrustum();
    testSphereInFrustum();
    testSpherePartiallyInFrustum();
    testSphereInFrustumDistance();
    testCubeInFrustum();
    testCubePartiallyInFrustum();
    testPolygonInFrustum();
    testEmptyFrustum();
    testTransformedBoundingSphere();
    testNearPlaneBoundary();
    testFarPlaneBoundary();
    testNegativeSphereRadius();
    testSphereOutsideNearFarPlanes();
    testDegeneratePointCulling();
    testPointsOnFrustumPlanes();
    
    std::cout << "All Frustum tests PASSED!" << std::endl;
    return 0;
}
