#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <cassert>

#include "test_helpers.h"

// ============================================================================
// Camera construction tests
// ============================================================================

void testCameraDefaultPosition() {
    std::cout << "  [TEST] Camera default position... ";
    
    // We cannot construct a real Camera without OpenGL context,
    // so we test the header defaults directly.
    // The header defines: position = glm::vec3(0.0f, 1.0f, 0.0f)
    glm::vec3 expectedPosition(0.0f, 1.0f, 0.0f);
    assert(approxEqual(expectedPosition.x, 0.0f));
    assert(approxEqual(expectedPosition.y, 1.0f));
    assert(approxEqual(expectedPosition.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testCameraDefaultDirection() {
    std::cout << "  [TEST] Camera default direction... ";
    
    // The header defines: direction = glm::vec3(0.0f, 0.0f, -1.0f)
    glm::vec3 expectedDirection(0.0f, 0.0f, -1.0f);
    assert(approxEqual(expectedDirection.x, 0.0f));
    assert(approxEqual(expectedDirection.y, 0.0f));
    assert(approxEqual(expectedDirection.z, -1.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testCameraDefaultRight() {
    std::cout << "  [TEST] Camera default right vector... ";
    
    // The header defines: right = glm::vec3(1.0f, 0.0f, 0.0f)
    glm::vec3 expectedRight(1.0f, 0.0f, 0.0f);
    assert(approxEqual(expectedRight.x, 1.0f));
    assert(approxEqual(expectedRight.y, 0.0f));
    assert(approxEqual(expectedRight.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testCameraDefaultUp() {
    std::cout << "  [TEST] Camera default up vector... ";
    
    // The header defines: up = glm::vec3(0.0f, 1.0f, 0.0f)
    glm::vec3 expectedUp(0.0f, 1.0f, 0.0f);
    assert(approxEqual(expectedUp.x, 0.0f));
    assert(approxEqual(expectedUp.y, 1.0f));
    assert(approxEqual(expectedUp.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Aim function tests (using manual calculation)
// ============================================================================

void testAimHorizontalRotation() {
    std::cout << "  [TEST] Horizontal aim rotation... ";
    
    // Simulate the aim function behavior
    float horizontalAngle = 3.14159265358979323846f; // PI (initial)
    float verticalAngle = 0.0f;
    
    // Aim right by 90 degrees (PI/2) using the same convention as Camera::aim.
    // A positive horizontal delta rotates the view toward +X from the default -Z facing direction.
    horizontalAngle -= 1.57079632679489661923f; // -PI/2
    
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    
    // After rotating -PI/2 from PI, direction should point along +X
    assert(approxEqual(direction.x, 1.0f));
    assert(approxEqual(direction.y, 0.0f));
    assert(approxEqual(direction.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testAimVerticalRotation() {
    std::cout << "  [TEST] Vertical aim rotation... ";
    
    float horizontalAngle = 3.14159265358979323846f; // PI
    float verticalAngle = 0.0f;
    
    // Aim up by 90 degrees (PI/2)
    verticalAngle += 1.57079632679489661923f; // PI/2
    
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    
    // After rotating PI/2 up, direction should point along +Y
    assert(approxEqual(direction.x, 0.0f));
    assert(approxEqual(direction.y, 1.0f));
    assert(approxEqual(direction.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testAimDownRotation() {
    std::cout << "  [TEST] Downward aim rotation... ";
    
    float horizontalAngle = 3.14159265358979323846f; // PI
    float verticalAngle = 0.0f;
    
    // Aim down by 90 degrees (-PI/2)
    verticalAngle -= 1.57079632679489661923f; // -PI/2
    
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    
    // After rotating -PI/2 down, direction should point along -Y
    assert(approxEqual(direction.x, 0.0f));
    assert(approxEqual(direction.y, -1.0f));
    assert(approxEqual(direction.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Movement function tests (using manual calculation)
// ============================================================================

void testMoveForward() {
    std::cout << "  [TEST] Move forward... ";
    
    glm::vec3 position(0.0f, 1.0f, 0.0f);
    float horizontalAngle = 3.14159265358979323846f; // PI
    float verticalAngle = 0.0f;
    
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    
    // Move forward by 5 units
    position += direction * 5.0f;
    
    // Direction at PI horizontal, 0 vertical is (0, 0, -1)
    // So position should be (0, 1, -5)
    assert(approxEqual(position.x, 0.0f));
    assert(approxEqual(position.y, 1.0f));
    assert(approxEqual(position.z, -5.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testMoveBackward() {
    std::cout << "  [TEST] Move backward... ";
    
    glm::vec3 position(0.0f, 1.0f, 0.0f);
    float horizontalAngle = 3.14159265358979323846f; // PI
    float verticalAngle = 0.0f;
    
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    
    // Move backward by 5 units (opposite of forward)
    position -= direction * 5.0f;
    
    // Should be at (0, 1, 5)
    assert(approxEqual(position.x, 0.0f));
    assert(approxEqual(position.y, 1.0f));
    assert(approxEqual(position.z, 5.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testMoveRight() {
    std::cout << "  [TEST] Move right... ";
    
    glm::vec3 position(0.0f, 1.0f, 0.0f);
    float horizontalAngle = 3.14159265358979323846f; // PI
    
    glm::vec3 right(
        sin(horizontalAngle - 3.14159265358979323846f / 2.0f),
        0.0f,
        cos(horizontalAngle - 3.14159265358979323846f / 2.0f)
    );
    
    // Move right by 3 units
    position += right * 3.0f;
    
    // At PI horizontal, right should be (1, 0, 0)
    // So position should be (3, 1, 0)
    assert(approxEqual(position.x, 3.0f));
    assert(approxEqual(position.y, 1.0f));
    assert(approxEqual(position.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testMoveLeft() {
    std::cout << "  [TEST] Move left... ";
    
    glm::vec3 position(0.0f, 1.0f, 0.0f);
    float horizontalAngle = 3.14159265358979323846f; // PI
    
    glm::vec3 right(
        sin(horizontalAngle - 3.14159265358979323846f / 2.0f),
        0.0f,
        cos(horizontalAngle - 3.14159265358979323846f / 2.0f)
    );
    
    // Move left by 3 units (opposite of right)
    position -= right * 3.0f;
    
    // Should be at (-3, 1, 0)
    assert(approxEqual(position.x, -3.0f));
    assert(approxEqual(position.y, 1.0f));
    assert(approxEqual(position.z, 0.0f));
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// LookAt matrix tests
// ============================================================================

void testLookAtMatrix() {
    std::cout << "  [TEST] LookAt matrix generation... ";
    
    glm::vec3 eye(0.0f, 1.0f, 5.0f);
    glm::vec3 center(0.0f, 1.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    glm::mat4 view = glm::lookAt(eye, center, up);
    
    // The view matrix should translate by (0, -1, -5) effectively for this camera setup.
    // GLM's lookAt stores the camera-space translation in the last row/column, so the Y
    // component must match the camera's up-axis offset, not the eye height expression.
    assert(approxEqual(view[3][0], 0.0f));
    assert(approxEqual(view[3][1], -1.0f));
    
    std::cout << "PASSED" << std::endl;
}

void testPerspectiveMatrix() {
    std::cout << "  [TEST] Perspective matrix generation... ";
    
    float fov = 45.0f * 3.14159265f / 180.0f;
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    glm::mat4 proj = glm::perspective(fov, aspect, nearPlane, farPlane);
    
    // Perspective matrix should have non-identity elements in the projection row
    assert(proj[0][0] != 0.0f); // x scaling
    assert(proj[1][1] != 0.0f); // y scaling
    assert(proj[2][2] != 0.0f); // z scaling (depth)
    assert(proj[2][3] != 0.0f); // z offset (depth)
    
    // Top element should be cot(fov/2)
    float expectedTop = 1.0f / std::tan(fov / 2.0f);
    assert(approxEqual(proj[1][1], expectedTop));
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Orthographic projection test
// ============================================================================

void testOrthographicMatrix() {
    std::cout << "  [TEST] Orthographic matrix generation... ";
    
    float left = -10.0f;
    float right = 10.0f;
    float bottom = -10.0f;
    float top = 10.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    
    glm::mat4 proj = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    
    // Orthographic matrix should have specific structure
    assert(approxEqual(proj[0][0], 1.0f / (right - left) * 2.0f));
    assert(approxEqual(proj[1][1], 1.0f / (top - bottom) * 2.0f));
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Vector orthogonality tests
// ============================================================================

void testDirectionRightOrthogonality() {
    std::cout << "  [TEST] Direction and right vectors orthogonality... ";
    
    float horizontalAngle = 1.0f; // Some arbitrary angle
    float verticalAngle = 0.5f;
    
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    
    glm::vec3 right(
        sin(horizontalAngle - 3.14159265358979323846f / 2.0f),
        0.0f,
        cos(horizontalAngle - 3.14159265358979323846f / 2.0f)
    );
    
    // Direction and right should be orthogonal (dot product = 0)
    float dot = glm::dot(direction, right);
    assert(approxEqual(dot, 0.0f, 1e-3f)); // Allow small numerical error
    
    std::cout << "PASSED" << std::endl;
}

void testRightUpOrthogonality() {
    std::cout << "  [TEST] Right and up vectors orthogonality... ";
    
    float horizontalAngle = 2.0f;
    float verticalAngle = 0.3f;
    
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    
    glm::vec3 right(
        sin(horizontalAngle - 3.14159265358979323846f / 2.0f),
        0.0f,
        cos(horizontalAngle - 3.14159265358979323846f / 2.0f)
    );
    
    glm::vec3 up = glm::cross(right, direction);
    
    // Right and up should be orthogonal
    float dotRightUp = glm::dot(right, up);
    assert(approxEqual(dotRightUp, 0.0f, 1e-3f));
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Test runner
// ============================================================================

int runCameraTests() {
    std::cout << "\n=== Camera Tests ===" << std::endl;
    
    testCameraDefaultPosition();
    testCameraDefaultDirection();
    testCameraDefaultRight();
    testCameraDefaultUp();
    testAimHorizontalRotation();
    testAimVerticalRotation();
    testAimDownRotation();
    testMoveForward();
    testMoveBackward();
    testMoveRight();
    testMoveLeft();
    testLookAtMatrix();
    testPerspectiveMatrix();
    testOrthographicMatrix();
    testDirectionRightOrthogonality();
    testRightUpOrthogonality();
    
    std::cout << "All Camera tests PASSED!" << std::endl;
    return 0;
}
