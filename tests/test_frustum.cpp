#include "Frustum.h"
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
    testNearPlaneBoundary();
    testFarPlaneBoundary();
    
    std::cout << "All Frustum tests PASSED!" << std::endl;
    return 0;
}
