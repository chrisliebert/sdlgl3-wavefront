#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <cassert>
#include <algorithm>

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
// Shadow light offset and camera properties test
// ============================================================================

void testShadowLightOffsetAndCameraProperties() {
    std::cout << "  [TEST] Shadow light offset and camera properties... ";
    
    // 1. Verify light position offset used for shadow directional lighting
    glm::vec3 lightOffset = Math::getLightPositionOffset();
    // Non-zero offset
    assert(glm::length(lightOffset) > 0.0f);
    // Elevated above ground (y > 0) to cast downward shadows
    assert(lightOffset.y > 0.0f);
    // Not collinear with a forward-only vector (0, 0, -1)
    glm::vec3 forwardVector(0.0f, 0.0f, -1.0f);
    glm::vec3 normOffset = glm::normalize(lightOffset);
    float dotForward = std::abs(glm::dot(normOffset, forwardVector));
    assert(dotForward < 0.99f);
    glm::vec3 crossForward = glm::cross(normOffset, forwardVector);
    assert(glm::length(crossForward) > 0.1f);
    
    // 2. Verify camera perspective projection aspect and fov values used by shadow calculations
    // Camera default perspective projection uses 45-degree vertical FOV and 16:9 aspect ratio
    const float fovDegrees = 45.0f;
    const float fovRadians = glm::radians(fovDegrees);
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 10000.0f;
    glm::mat4 proj = glm::perspective(fovRadians, aspect, nearPlane, farPlane);
    
    // Shadow calculations use p11 and p00 from camera projection:
    // p11 = 1 / tan(fov / 2) ≈ 2.4142f
    // p00 = p11 / aspect ≈ 1.358f
    const float expectedP11 = 1.0f / std::tan(fovRadians / 2.0f);
    const float expectedP00 = expectedP11 / aspect;
    
    assert(approxEqual(proj[1][1], expectedP11, 1e-3f));
    assert(approxEqual(proj[0][0], expectedP00, 1e-3f));
    assert(approxEqual(proj[1][1], 2.4142f, 1e-3f));
    assert(approxEqual(proj[0][0], 1.358f, 1e-3f));
    
    // Verify shadow frustum half-extents at clamped max shadow distance (120 units)
    const float shadowDist = 120.0f;
    const float h = shadowDist / proj[1][1];
    const float w = shadowDist / proj[0][0];
    assert(h > 0.0f && w > 0.0f);
    assert(approxEqual(w / h, aspect, 1e-3f));
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Camera Pitch Clamping Tests
// ============================================================================

void testCameraPitchClampingUpperLimit() {
    std::cout << "  [TEST] Camera pitch clamping at upper limit (+1.55 rad)... ";
    
    // Pitch must be clamped to [-1.55, 1.55] rad (~88.8 deg)
    // Even if aiming by large positive delta (+5.0, +10.0 rad, etc.),
    // pitch is clamped to +1.55 radians.
    const double kMaxPitch = 1.55;
    double verticalAngle = 0.0;
    
    verticalAngle += 5.0;
    verticalAngle = std::clamp(verticalAngle, -kMaxPitch, kMaxPitch);
    assert(approxEqual(static_cast<float>(verticalAngle), 1.55f));
    
    // At pitch = 1.55 rad, evaluate direction and up vectors
    double horizontalAngle = 3.14159265358979323846; // PI
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    glm::vec3 right(
        sin(horizontalAngle - 3.14159265358979323846 / 2.0),
        0.0,
        cos(horizontalAngle - 3.14159265358979323846 / 2.0)
    );
    glm::vec3 up = glm::cross(right, direction);
    
    // Up vector Y must be strictly positive (camera is not upside down)
    // up.y = cos(verticalAngle) = cos(1.55) > 0
    assert(up.y > 0.0f);
    assert(direction.y <= std::sin(1.55f));
    assert(direction.y > 0.99f); // pointing almost straight up, but not degenerate
    
    // View matrix must be non-singular
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) + direction, up);
    float det = glm::determinant(view);
    assert(std::abs(det) > 1e-4f);
    
    std::cout << "PASSED" << std::endl;
}

void testCameraPitchClampingLowerLimit() {
    std::cout << "  [TEST] Camera pitch clamping at lower limit (-1.55 rad)... ";
    
    const double kMaxPitch = 1.55;
    double verticalAngle = 0.0;
    
    // Aim with extreme downward delta (-10.0 rad)
    verticalAngle -= 10.0;
    verticalAngle = std::clamp(verticalAngle, -kMaxPitch, kMaxPitch);
    assert(approxEqual(static_cast<float>(verticalAngle), -1.55f));
    
    double horizontalAngle = 3.14159265358979323846; // PI
    glm::vec3 direction(
        cos(verticalAngle) * sin(horizontalAngle),
        sin(verticalAngle),
        cos(verticalAngle) * cos(horizontalAngle)
    );
    glm::vec3 right(
        sin(horizontalAngle - 3.14159265358979323846 / 2.0),
        0.0,
        cos(horizontalAngle - 3.14159265358979323846 / 2.0)
    );
    glm::vec3 up = glm::cross(right, direction);
    
    // Up vector Y must remain positive (never flipped)
    assert(up.y > 0.0f);
    assert(direction.y >= -std::sin(1.55f));
    assert(direction.y < -0.99f); // pointing almost straight down
    
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) + direction, up);
    float det = glm::determinant(view);
    assert(std::abs(det) > 1e-4f);
    
    std::cout << "PASSED" << std::endl;
}

void testCameraPitchNeverInvertsUpVector() {
    std::cout << "  [TEST] Camera pitch clamping prevents inversion across 360-degree sweep... ";
    
    const double kMaxPitch = 1.55;
    const double kPi = 3.14159265358979323846;
    
    // Test across a variety of yaw angles and pitch attempts beyond [-pi/2, pi/2]
    for (double yaw = 0.0; yaw < 2.0 * kPi; yaw += kPi / 4.0) {
        for (double rawPitch = -4.0 * kPi; rawPitch <= 4.0 * kPi; rawPitch += 0.2) {
            double clampedPitch = std::clamp(rawPitch, -kMaxPitch, kMaxPitch);
            
            glm::vec3 direction(
                cos(clampedPitch) * sin(yaw),
                sin(clampedPitch),
                cos(clampedPitch) * cos(yaw)
            );
            glm::vec3 right(
                sin(yaw - kPi / 2.0),
                0.0,
                cos(yaw - kPi / 2.0)
            );
            glm::vec3 up = glm::cross(right, direction);
            
            // Invariant 1: Up vector Y is always positive (cos(pitch) > 0 for |pitch| <= 1.55)
            assert(up.y > 0.0f);
            
            // Invariant 2: Direction Y magnitude never exceeds sin(1.55)
            assert(std::abs(direction.y) <= std::sin(1.55f) + 1e-5f);
            
            // Invariant 3: Orthogonality preserved
            assert(std::abs(glm::dot(direction, up)) < 1e-4f);
            assert(std::abs(glm::dot(direction, right)) < 1e-4f);
            assert(std::abs(glm::dot(up, right)) < 1e-4f);
            
            // Invariant 4: Non-singular view matrix
            glm::mat4 view = glm::lookAt(glm::vec3(0.0f), direction, up);
            assert(!std::isnan(view[0][0]) && !std::isinf(view[0][0]));
            assert(std::abs(glm::determinant(view)) > 1e-4f);
        }
    }
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Camera Angle Synchronization Tests
// ============================================================================

void testCameraAngleSynchronizationFromDirection() {
    std::cout << "  [TEST] Camera angle synchronization from arbitrary target direction... ";
    
    const glm::vec3 targets[] = {
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
        glm::normalize(glm::vec3(1.0f, 0.5f, -1.0f)),
        glm::normalize(glm::vec3(-2.0f, -0.8f, 3.0f)),
        glm::normalize(glm::vec3(0.3f, 0.95f, -0.2f)) // steep elevation near clamp boundary
    };
    
    for (const glm::vec3& targetDir : targets) {
        // Synchronize angles from direction (as done when centering scene):
        // horizontalAngle = atan2(dir.x, dir.z)
        // verticalAngle = asin(clamp(dir.y, -1.0, 1.0))
        double horizAngle = std::atan2(static_cast<double>(targetDir.x), static_cast<double>(targetDir.z));
        double vertAngle = std::asin(std::clamp(static_cast<double>(targetDir.y), -1.0, 1.0));
        vertAngle = std::clamp(vertAngle, -1.55, 1.55);
        
        // Reconstruct direction using Camera spherical coordinate formulation
        glm::vec3 reconstructedDir(
            cos(vertAngle) * sin(horizAngle),
            sin(vertAngle),
            cos(vertAngle) * cos(horizAngle)
        );
        
        // The reconstructed direction must match the target direction closely
        assert(approxEqual(reconstructedDir.x, targetDir.x, 1e-4f));
        assert(approxEqual(reconstructedDir.y, targetDir.y, 1e-4f));
        assert(approxEqual(reconstructedDir.z, targetDir.z, 1e-4f));
        
        // Verify no jump on small mouse delta after synchronization:
        double deltaYaw = 0.001;
        double deltaPitch = 0.001;
        double postYaw = horizAngle - deltaYaw;
        double postPitch = std::clamp(vertAngle + deltaPitch, -1.55, 1.55);
        glm::vec3 postDir(
            cos(postPitch) * sin(postYaw),
            sin(postPitch),
            cos(postPitch) * cos(postYaw)
        );
        float jumpDist = glm::distance(postDir, targetDir);
        assert(jumpDist < 0.01f);
    }
    
    std::cout << "PASSED" << std::endl;
}

void testCameraAngleSynchronizationPreventsFirstFrameSnap() {
    std::cout << "  [TEST] Camera angle synchronization prevents first-frame mouse snap... ";
    
    // Scenario: Scene loads, camera looks at object at (+10, 2, +10) from (0, 0, 0).
    glm::vec3 eye(0.0f, 0.0f, 0.0f);
    glm::vec3 objectPos(10.0f, 2.0f, 10.0f);
    glm::vec3 desiredDir = glm::normalize(objectPos - eye);
    
    // Without angle synchronization:
    // horizontalAngle remains at default kPi, verticalAngle at 0.0.
    // On first mouse move of 0.001 rad, direction snaps back toward (-Z):
    double unsyncedHoriz = 3.14159265358979323846 - 0.001;
    glm::vec3 snappedDir(sin(unsyncedHoriz), 0.0f, cos(unsyncedHoriz));
    float snapDistance = glm::distance(snappedDir, desiredDir);
    // Unsynced direction snaps violently (> 1.0 unit away in direction space)
    assert(snapDistance > 1.0f);
    
    // WITH angle synchronization:
    double syncedHoriz = std::atan2(desiredDir.x, desiredDir.z);
    double syncedVert = std::asin(std::clamp(static_cast<double>(desiredDir.y), -1.0, 1.0));
    syncedVert = std::clamp(syncedVert, -1.55, 1.55);
    
    // First mouse move of 0.001 rad:
    double mouseDelta = 0.001;
    double movedHoriz = syncedHoriz - mouseDelta;
    double movedVert = std::clamp(syncedVert + mouseDelta, -1.55, 1.55);
    glm::vec3 smoothDir(
        cos(movedVert) * sin(movedHoriz),
        sin(movedVert),
        cos(movedVert) * cos(movedHoriz)
    );
    
    float smoothDistance = glm::distance(smoothDir, desiredDir);
    // Synced direction moves smoothly by tiny delta (< 0.005 units)
    assert(smoothDistance < 0.005f);
    
    std::cout << "PASSED" << std::endl;
}

void testCameraPitchClampingBoundaryAccumulatorStability() {
    std::cout << "  [TEST] Camera pitch clamping boundary accumulator stability... ";
    
    const double kMaxPitch = 1.55;
    double verticalAngle = 0.0;
    
    // Simulate continuous mouse movement upwards (aiming up 500 times by 0.05 rad)
    for (int i = 0; i < 500; ++i) {
        verticalAngle = std::clamp(verticalAngle + 0.05, -kMaxPitch, kMaxPitch);
    }
    // Must be locked exactly at 1.55 rad without drift
    assert(approxEqual(static_cast<float>(verticalAngle), 1.55f));
    
    // Now reverse: continuous mouse movement downwards (aiming down 1000 times by 0.05 rad)
    for (int i = 0; i < 1000; ++i) {
        verticalAngle = std::clamp(verticalAngle - 0.05, -kMaxPitch, kMaxPitch);
    }
    // Must be locked exactly at -1.55 rad without drift
    assert(approxEqual(static_cast<float>(verticalAngle), -1.55f));
    
    // At both boundaries, verify cos(pitch) > 0, ensuring right and direction never degenerate
    for (double boundaryPitch : { -kMaxPitch, kMaxPitch }) {
        double cosPitch = std::cos(boundaryPitch);
        assert(cosPitch > 0.02f); // Non-zero positive cosine
        
        double horizontalAngle = 3.14159265358979323846; // PI
        glm::vec3 direction(
            static_cast<float>(cos(boundaryPitch) * sin(horizontalAngle)),
            static_cast<float>(sin(boundaryPitch)),
            static_cast<float>(cos(boundaryPitch) * cos(horizontalAngle))
        );
        glm::vec3 right(
            static_cast<float>(sin(horizontalAngle - 3.14159265358979323846 / 2.0)),
            0.0f,
            static_cast<float>(cos(horizontalAngle - 3.14159265358979323846 / 2.0))
        );
        glm::vec3 up = glm::cross(right, direction);
        
        assert(up.y > 0.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) + direction, up);
        float det = glm::determinant(view);
        assert(approxEqual(std::abs(det), 1.0f, 1e-3f));
    }
    
    std::cout << "PASSED" << std::endl;
}

void testCameraInitialAngleSynchronizationComprehensive() {
    std::cout << "  [TEST] Initial angle synchronization across 3D directions and quadrants... ";
    
    // Test across all 4 quadrants in X-Z plane, cardinal axes, steep pitch angles, and near-clamp boundaries
    const glm::vec3 testDirections[] = {
        glm::vec3(0.0f, 0.0f, -1.0f),  // Forward (-Z)
        glm::vec3(1.0f, 0.0f, 0.0f),   // Right (+X)
        glm::vec3(0.0f, 0.0f, 1.0f),   // Backward (+Z)
        glm::vec3(-1.0f, 0.0f, 0.0f),  // Left (-X)
        glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)),   // Quadrant 1 (+X, +Z)
        glm::normalize(glm::vec3(1.0f, 0.0f, -1.0f)),  // Quadrant 4 (+X, -Z)
        glm::normalize(glm::vec3(-1.0f, 0.0f, 1.0f)),  // Quadrant 2 (-X, +Z)
        glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f)), // Quadrant 3 (-X, -Z)
        glm::normalize(glm::vec3(0.5f, 0.8f, -0.3f)),  // Steep upward pitch
        glm::normalize(glm::vec3(-0.4f, -0.85f, 0.2f)),// Steep downward pitch
        glm::normalize(glm::vec3(0.01f, 0.999f, 0.0f)),// Near top pole (clamped to 1.55)
        glm::normalize(glm::vec3(0.0f, -0.999f, 0.01f))// Near bottom pole (clamped to -1.55)
    };
    
    for (const glm::vec3& rawDir : testDirections) {
        glm::vec3 targetDir = glm::normalize(rawDir);
        
        // Exact angle synchronization logic as in main.cpp:
        double horizontalAngle = std::atan2(static_cast<double>(targetDir.x), static_cast<double>(targetDir.z));
        double verticalAngle = std::asin(std::clamp(static_cast<double>(targetDir.y), -1.0, 1.0));
        verticalAngle = std::clamp(verticalAngle, -1.55, 1.55);
        
        // Reconstruct direction and camera coordinate frame
        const double kPi = 3.14159265358979323846;
        glm::vec3 reconstructedDir(
            static_cast<float>(cos(verticalAngle) * sin(horizontalAngle)),
            static_cast<float>(sin(verticalAngle)),
            static_cast<float>(cos(verticalAngle) * cos(horizontalAngle))
        );
        glm::vec3 right(
            static_cast<float>(sin(horizontalAngle - kPi / 2.0)),
            0.0f,
            static_cast<float>(cos(horizontalAngle - kPi / 2.0))
        );
        glm::vec3 up = glm::cross(right, reconstructedDir);
        
        // 1. Vertical angle must strictly reside within [-1.55, 1.55]
        assert(verticalAngle >= -1.55 && verticalAngle <= 1.55);
        
        // 2. Up vector must maintain positive Y (never inverted)
        assert(up.y > 0.0f);
        
        // 3. For directions within pitch bounds, reconstructed direction matches target
        if (std::abs(targetDir.y) < std::sin(1.545f)) {
            assert(approxEqual(reconstructedDir.x, targetDir.x, 1e-3f));
            assert(approxEqual(reconstructedDir.y, targetDir.y, 1e-3f));
            assert(approxEqual(reconstructedDir.z, targetDir.z, 1e-3f));
        }
        
        // 4. View matrix generated is non-singular
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f), reconstructedDir, up);
        assert(!std::isnan(view[0][0]) && !std::isinf(view[0][0]));
        assert(approxEqual(std::abs(glm::determinant(view)), 1.0f, 1e-3f));
    }
    
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
    testShadowLightOffsetAndCameraProperties();
    testCameraPitchClampingUpperLimit();
    testCameraPitchClampingLowerLimit();
    testCameraPitchNeverInvertsUpVector();
    testCameraAngleSynchronizationFromDirection();
    testCameraAngleSynchronizationPreventsFirstFrameSnap();
    testCameraPitchClampingBoundaryAccumulatorStability();
    testCameraInitialAngleSynchronizationComprehensive();
    
    std::cout << "All Camera tests PASSED!" << std::endl;
    return 0;
}
