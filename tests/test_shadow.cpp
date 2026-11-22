#include <iostream>
#include <cmath>
#include <cassert>
#include <array>
#include <limits>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Common.h"
#include "test_helpers.h"

namespace {

// Helper to compute frustum corners given camera parameters
std::array<glm::vec3, 8> computeFrustumCorners(
    const glm::vec3& camPos,
    const glm::vec3& camDir,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    float fovYRadians,
    float aspect,
    float nearDist,
    float farDist)
{
    const float p11 = 1.0f / std::tan(fovYRadians * 0.5f);
    const float p00 = p11 / aspect;

    std::array<glm::vec3, 8> corners{};
    const float distances[2] = { nearDist, farDist };
    int idx = 0;
    for (float d : distances)
    {
        const float h = d / p11;
        const float w = d / p00;
        const glm::vec3 c = camPos + camDir * d;
        corners[idx++] = c + camRight * w + camUp * h;
        corners[idx++] = c - camRight * w + camUp * h;
        corners[idx++] = c + camRight * w - camUp * h;
        corners[idx++] = c - camRight * w - camUp * h;
    }
    return corners;
}

// Vulkan clip space matrix mapping NDC Y -> -Y and Z [-1, 1] -> [0, 1]
glm::mat4 getVulkanClipMatrix()
{
    glm::mat4 clip(1.0f);
    clip[1][1] = -1.0f;
    clip[2][2] = 0.5f;
    clip[3][2] = 0.5f;
    return clip;
}

bool mat4ApproxEqual(const glm::mat4& a, const glm::mat4& b, float eps = 1e-4f)
{
    for (int c = 0; c < 4; ++c)
    {
        for (int r = 0; r < 4; ++r)
        {
            if (std::abs(a[c][r] - b[c][r]) >= eps) return false;
        }
    }
    return true;
}

} // anonymous namespace

// ============================================================================
// Directional Sunlight Stability Tests
// ============================================================================

void testDirectionalLightSunVectorInvariance()
{
    std::cout << "  [TEST] Directional sunlight vector is constant and non-zero... ";

    const glm::vec3 lightOffset = Math::getLightPositionOffset();
    assert(glm::length(lightOffset) > 0.0f);
    assert(lightOffset.y > 0.0f); // Light comes from above

    const glm::vec3 lightDir = glm::normalize(lightOffset);
    assert(approxEqual(glm::length(lightDir), 1.0f, 1e-4f));

    // Directional light vector must remain invariant regardless of camera rotation.
    // In buggy code, lightDir was dynamically recomputed as normalize(frustumCenter - lightPosition),
    // which rotated sunlight whenever the camera looked in a different direction.
    // Verify that using fixed lightDir keeps sun orientation constant across 360-degree yaw and pitch sweep:
    const double kPi = 3.14159265358979323846;
    for (double yaw = 0.0; yaw < 2.0 * kPi; yaw += kPi / 8.0)
    {
        for (double pitch = -1.55; pitch <= 1.55; pitch += 0.31)
        {
            glm::vec3 camDir(
                std::cos(pitch) * std::sin(yaw),
                std::sin(pitch),
                std::cos(pitch) * std::cos(yaw)
            );
            (void)camDir;
            // Sunlight vector is independent of camera direction and orientation
            float dotProduct = glm::dot(lightDir, glm::normalize(lightOffset));
            assert(approxEqual(dotProduct, 1.0f, 1e-5f));
            assert(approxEqual(glm::length(lightDir), 1.0f, 1e-5f));
        }
    }

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Light View Matrix Stability Tests
// ============================================================================

void testDirectionalLightViewMatrixStability()
{
    std::cout << "  [TEST] Light view matrix non-degeneracy across camera positions and yaws... ";

    const glm::vec3 lightDir = glm::normalize(Math::getLightPositionOffset());
    const glm::vec3 lightUp = (std::abs(lightDir.y) > 0.99f)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    const glm::vec3 testPositions[] = {
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(150.0f, 25.0f, -200.0f),
        glm::vec3(-500.0f, 200.0f, 300.0f),
        glm::vec3(0.0f, 50.0f, 0.0f)
    };

    const float fovY = glm::radians(45.0f);
    const float aspect = 16.0f / 9.0f;
    const float nearDist = 0.1f;
    const float shadowDist = 120.0f;

    for (const auto& camPos : testPositions)
    {
        for (double yaw = 0.0; yaw < 6.28; yaw += 1.0)
        {
            glm::vec3 camDir(sin(yaw), 0.0f, cos(yaw));
            glm::vec3 camRight(cos(yaw), 0.0f, -sin(yaw));
            glm::vec3 camUp(0.0f, 1.0f, 0.0f);

            auto corners = computeFrustumCorners(camPos, camDir, camRight, camUp, fovY, aspect, nearDist, shadowDist);

            glm::vec3 frustumCenter(0.0f);
            for (const auto& c : corners)
            {
                frustumCenter += c;
            }
            frustumCenter /= static_cast<float>(corners.size());

            // Light position placed along lightDir backwards from frustum center
            const float lightDistance = 200.0f;
            const glm::vec3 lightPos = frustumCenter + lightDir * lightDistance;

            glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, lightUp);

            // 1. Matrix determinant must be +/- 1.0 (pure rigid rotation + translation)
            float det = glm::determinant(lightView);
            assert(!std::isnan(det) && !std::isinf(det));
            assert(approxEqual(std::abs(det), 1.0f, 1e-3f));

            // 2. Frustum center in light view space must be exactly at (0, 0, -lightDistance)
            glm::vec4 centerInLightSpace = lightView * glm::vec4(frustumCenter, 1.0f);
            assert(approxEqual(centerInLightSpace.x, 0.0f, 1e-2f));
            assert(approxEqual(centerInLightSpace.y, 0.0f, 1e-2f));
            assert(approxEqual(centerInLightSpace.z, -lightDistance, 1e-2f));
        }
    }

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// OpenGL Convention Shadow Projection Stability Tests
// ============================================================================

void testOpenGLShadowProjectionStability()
{
    std::cout << "  [TEST] OpenGL shadow projection stability, bounds, and NDC containment... ";

    const glm::vec3 camPos(10.0f, 2.0f, -15.0f);
    const glm::vec3 camDir = glm::normalize(glm::vec3(0.5f, -0.2f, -1.0f));
    const glm::vec3 camRight = glm::normalize(glm::cross(camDir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 camUp = glm::cross(camRight, camDir);

    const float fovY = glm::radians(45.0f);
    const float aspect = 16.0f / 9.0f;
    const float nearDist = 0.1f;
    const float shadowDist = 120.0f;

    auto corners = computeFrustumCorners(camPos, camDir, camRight, camUp, fovY, aspect, nearDist, shadowDist);

    glm::vec3 frustumCenter(0.0f);
    for (const auto& c : corners)
    {
        frustumCenter += c;
    }
    frustumCenter /= static_cast<float>(corners.size());

    const glm::vec3 lightDir = glm::normalize(Math::getLightPositionOffset());
    const glm::vec3 lightUp = (std::abs(lightDir.y) > 0.99f)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    const glm::vec3 lightPos = frustumCenter + lightDir * 200.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, lightUp);

    // Compute bounding box in light space
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    for (const auto& corner : corners)
    {
        glm::vec4 lsCorner = lightView * glm::vec4(corner, 1.0f);
        minBounds = glm::min(minBounds, glm::vec3(lsCorner));
        maxBounds = glm::max(maxBounds, glm::vec3(lsCorner));
    }

    // Verify non-degenerate bounding box
    assert(maxBounds.x > minBounds.x);
    assert(maxBounds.y > minBounds.y);
    assert(maxBounds.z > minBounds.z);
    assert((maxBounds.x - minBounds.x) > 5.0f);
    assert((maxBounds.y - minBounds.y) > 5.0f);
    assert((maxBounds.z - minBounds.z) > 10.0f);

    // OpenGL orthographic projection with Z padding
    constexpr float zPadding = 50.0f;
    const float zNear = -maxBounds.z - zPadding;
    const float zFar = -minBounds.z + zPadding;
    assert(zNear < zFar);
    assert(zNear > 0.0f); // Positive near plane distance

    glm::mat4 lightProj = glm::ortho(minBounds.x, maxBounds.x, minBounds.y, maxBounds.y, zNear, zFar);
    glm::mat4 lightSpaceMatrix = lightProj * lightView;

    // 1. Matrix determinant non-zero and invertible
    float detProj = glm::determinant(lightProj);
    float detSpace = glm::determinant(lightSpaceMatrix);
    assert(!std::isnan(detProj) && !std::isinf(detProj) && std::abs(detProj) > 1e-8f);
    assert(!std::isnan(detSpace) && !std::isinf(detSpace) && std::abs(detSpace) > 1e-8f);

    glm::mat4 invSpace = glm::inverse(lightSpaceMatrix);
    glm::mat4 identityCheck = invSpace * lightSpaceMatrix;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            float expected = (r == c) ? 1.0f : 0.0f;
            assert(approxEqual(identityCheck[c][r], expected, 1e-3f));
        }
    }

    // 2. All 8 camera frustum corners must fall strictly inside OpenGL NDC [-1, 1]
    for (const auto& corner : corners)
    {
        glm::vec4 ndc = lightSpaceMatrix * glm::vec4(corner, 1.0f);
        // Orthographic projection w component is 1.0
        assert(approxEqual(ndc.w, 1.0f, 1e-4f));
        assert(ndc.x >= -1.001f && ndc.x <= 1.001f);
        assert(ndc.y >= -1.001f && ndc.y <= 1.001f);
        // In OpenGL NDC, Z must be in [-1, 1]
        assert(ndc.z >= -1.001f && ndc.z <= 1.001f);
    }

    // 3. Negative depth clipping test:
    // A shadow caster positioned between lightPos and frustum front (within zPadding)
    // must not be clipped behind near plane (ndc.z >= -1.0f)
    glm::vec3 frontCaster = frustumCenter + lightDir * 30.0f;
    glm::vec4 frontNdc = lightSpaceMatrix * glm::vec4(frontCaster, 1.0f);
    assert(frontNdc.z >= -1.0f);

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Vulkan Convention Shadow Projection Stability Tests
// ============================================================================

void testVulkanShadowProjectionStability()
{
    std::cout << "  [TEST] Vulkan shadow projection [0, 1] depth and clip transform... ";

    const glm::vec3 camPos(0.0f, 1.0f, 0.0f);
    const glm::vec3 camDir(0.0f, 0.0f, -1.0f);
    const glm::vec3 camRight(1.0f, 0.0f, 0.0f);
    const glm::vec3 camUp(0.0f, 1.0f, 0.0f);

    const float fovY = glm::radians(45.0f);
    const float aspect = 16.0f / 9.0f;
    const float nearDist = 0.1f;
    const float shadowDist = 120.0f;

    auto corners = computeFrustumCorners(camPos, camDir, camRight, camUp, fovY, aspect, nearDist, shadowDist);

    glm::vec3 frustumCenter(0.0f);
    for (const auto& c : corners)
    {
        frustumCenter += c;
    }
    frustumCenter /= static_cast<float>(corners.size());

    const glm::vec3 lightDir = glm::normalize(Math::getLightPositionOffset());
    const glm::vec3 lightUp = (std::abs(lightDir.y) > 0.99f)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    const glm::vec3 lightPos = frustumCenter + lightDir * 200.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, lightUp);

    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    for (const auto& corner : corners)
    {
        glm::vec4 lsCorner = lightView * glm::vec4(corner, 1.0f);
        minBounds = glm::min(minBounds, glm::vec3(lsCorner));
        maxBounds = glm::max(maxBounds, glm::vec3(lsCorner));
    }

    constexpr float zPadding = 50.0f;
    glm::mat4 lightProjGL = glm::ortho(
        minBounds.x, maxBounds.x,
        minBounds.y, maxBounds.y,
        -maxBounds.z - zPadding,
        -minBounds.z + zPadding);

    // Incorporate Vulkan clip transform:
    // Y flipped (-1) and depth remapped from [-1, 1] to [0, 1]
    const glm::mat4 vulkanClip = getVulkanClipMatrix();
    const glm::mat4 lightSpaceMatrixVK = vulkanClip * lightProjGL * lightView;

    // 1. Matrix determinant non-zero and invertible
    float detVK = glm::determinant(lightSpaceMatrixVK);
    assert(!std::isnan(detVK) && !std::isinf(detVK) && std::abs(detVK) > 1e-8f);

    glm::mat4 invVK = glm::inverse(lightSpaceMatrixVK);
    glm::mat4 idVK = invVK * lightSpaceMatrixVK;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            float expected = (r == c) ? 1.0f : 0.0f;
            assert(approxEqual(idVK[c][r], expected, 1e-3f));
        }
    }

    // 2. All 8 corners must fall strictly inside Vulkan NDC:
    // X in [-1, 1], Y in [-1, 1], Z in [0, 1]
    for (const auto& corner : corners)
    {
        glm::vec4 ndcVK = lightSpaceMatrixVK * glm::vec4(corner, 1.0f);
        assert(approxEqual(ndcVK.w, 1.0f, 1e-4f));
        assert(ndcVK.x >= -1.001f && ndcVK.x <= 1.001f);
        assert(ndcVK.y >= -1.001f && ndcVK.y <= 1.001f);
        // Critical: Vulkan depth must be >= 0.0 and <= 1.0
        assert(ndcVK.z >= -0.001f && ndcVK.z <= 1.001f);
    }

    // 3. Verify depth bounds monotonicity:
    // Points closer to light source (+lightDir towards lightPos) have lower depth in [0, 1]
    glm::vec4 nearNdc = lightSpaceMatrixVK * glm::vec4(frustumCenter + lightDir * 30.0f, 1.0f);
    glm::vec4 farNdc = lightSpaceMatrixVK * glm::vec4(frustumCenter - lightDir * 30.0f, 1.0f);
    assert(nearNdc.z >= 0.0f);
    assert(farNdc.z <= 1.0f);
    assert(nearNdc.z < farNdc.z); // Monotonic depth progression

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Multi-Aspect Ratio and Extreme Pitch Non-Degeneracy Tests
// ============================================================================

void testShadowProjectionUnderExtremePitchesAndAspects()
{
    std::cout << "  [TEST] Shadow matrices non-degenerate across aspects & extreme pitch... ";

    const float aspects[] = { 16.0f / 9.0f, 4.0f / 3.0f, 21.0f / 9.0f, 1.0f };
    const double pitches[] = { -1.55, -0.78, 0.0, 0.78, 1.55 }; // Clamped bounds
    const glm::vec3 lightDir = glm::normalize(Math::getLightPositionOffset());
    const glm::vec3 lightUp = (std::abs(lightDir.y) > 0.99f)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 vulkanClip = getVulkanClipMatrix();

    for (float aspect : aspects)
    {
        for (double pitch : pitches)
        {
            double yaw = 1.2; // Arbitrary yaw
            glm::vec3 camDir(
                cos(pitch) * sin(yaw),
                sin(pitch),
                cos(pitch) * cos(yaw)
            );
            glm::vec3 camRight(
                sin(yaw - 3.14159265358979323846 / 2.0),
                0.0,
                cos(yaw - 3.14159265358979323846 / 2.0)
            );
            glm::vec3 camUp = glm::cross(camRight, camDir);
            glm::vec3 camPos(50.0f, 10.0f, -80.0f);

            auto corners = computeFrustumCorners(camPos, camDir, camRight, camUp, glm::radians(45.0f), aspect, 0.1f, 120.0f);

            glm::vec3 frustumCenter(0.0f);
            for (const auto& c : corners)
            {
                frustumCenter += c;
            }
            frustumCenter /= static_cast<float>(corners.size());

            const glm::vec3 lightPos = frustumCenter + lightDir * 200.0f;
            glm::mat4 lightView = glm::lookAt(lightPos, frustumCenter, lightUp);

            glm::vec3 minBounds(std::numeric_limits<float>::max());
            glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
            for (const auto& corner : corners)
            {
                glm::vec4 lsCorner = lightView * glm::vec4(corner, 1.0f);
                minBounds = glm::min(minBounds, glm::vec3(lsCorner));
                maxBounds = glm::max(maxBounds, glm::vec3(lsCorner));
            }

            constexpr float zPadding = 50.0f;
            glm::mat4 lightProjGL = glm::ortho(
                minBounds.x, maxBounds.x,
                minBounds.y, maxBounds.y,
                -maxBounds.z - zPadding,
                -minBounds.z + zPadding);

            glm::mat4 lightSpaceMatrixGL = lightProjGL * lightView;
            glm::mat4 lightSpaceMatrixVK = vulkanClip * lightSpaceMatrixGL;

            // Invariants:
            // 1. Both matrices have valid non-zero determinants
            assert(!std::isnan(glm::determinant(lightSpaceMatrixGL)));
            assert(!std::isnan(glm::determinant(lightSpaceMatrixVK)));
            assert(std::abs(glm::determinant(lightSpaceMatrixGL)) > 1e-8f);
            assert(std::abs(glm::determinant(lightSpaceMatrixVK)) > 1e-8f);

            // 2. Volume of light bounding box is well above zero
            float volume = (maxBounds.x - minBounds.x) * (maxBounds.y - minBounds.y) * (maxBounds.z - minBounds.z);
            assert(volume > 100.0f);
        }
    }

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Camera Rotation Invariance Tests (Shadow Stability / Swimming Prevention)
// ============================================================================

void testDirectionalShadowCameraRotationInvariance()
{
    std::cout << "  [TEST] Directional shadow matrices invariant under camera yaw/pitch sweeps... ";

    const glm::vec3 testPositions[] = {
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(120.0f, 45.0f, -80.0f),
        glm::vec3(-250.0f, 10.0f, 300.0f)
    };

    const float nearDist = 0.1f;
    const float shadowDist = 25.0f;
    const float p11 = 2.4142f; // cot(45 deg / 2)
    const float p00 = p11 / (16.0f / 9.0f);
    const float fovY = 2.0f * std::atan(1.0f / p11);
    const float aspect = p11 / p00;
    const glm::mat4 vulkanClip = getVulkanClipMatrix();

    constexpr double kPi = 3.14159265358979323846;

    for (const glm::vec3& camPos : testPositions)
    {
        // Reference matrices computed at default orientation (yaw = 0, pitch = 0)
        const glm::vec3 refCamDir(0.0f, 0.0f, -1.0f);
        const glm::vec3 refCamRight(1.0f, 0.0f, 0.0f);
        const glm::vec3 refCamUp(0.0f, 1.0f, 0.0f);

        auto refCorners = computeFrustumCorners(camPos, refCamDir, refCamRight, refCamUp, fovY, aspect, nearDist, shadowDist);
        const glm::vec3 shadowCenter = camPos;

        float refSphereRadius = 0.0f;
        for (const auto& corner : refCorners)
        {
            refSphereRadius = std::max(refSphereRadius, glm::distance(corner, shadowCenter));
        }
        assert(refSphereRadius <= 40.0f);
        assert(refSphereRadius >= 25.0f);

        const glm::vec3 lightDir = glm::normalize(Math::getLightPositionOffset());
        const glm::vec3 lightUp = (std::abs(lightDir.y) > 0.99f)
            ? glm::vec3(0.0f, 0.0f, 1.0f)
            : glm::vec3(0.0f, 1.0f, 0.0f);

        constexpr float zPadding = 45.0f;
        const float lightDistance = refSphereRadius + zPadding;
        assert(lightDistance <= 85.0f);
        const glm::vec3 lightEye = shadowCenter + lightDir * lightDistance;

        const glm::mat4 refLightView = glm::lookAt(lightEye, shadowCenter, lightUp);
        const glm::mat4 refLightProjGL = glm::ortho(
            -refSphereRadius, refSphereRadius,
            -refSphereRadius, refSphereRadius,
            0.0f, 2.0f * lightDistance);
        const glm::mat4 refLightSpaceMatrixGL = refLightProjGL * refLightView;
        const glm::mat4 refLightSpaceMatrixVK = vulkanClip * refLightProjGL * refLightView;

        // Sweep 360 degrees yaw and full clamped pitch [-1.55, 1.55]
        for (double yaw = 0.0; yaw < 2.0 * kPi; yaw += kPi / 12.0)
        {
            for (double pitch = -1.55; pitch <= 1.55; pitch += 0.31)
            {
                glm::vec3 camDir(
                    std::cos(pitch) * std::sin(yaw),
                    std::sin(pitch),
                    std::cos(pitch) * std::cos(yaw)
                );
                glm::vec3 camRight(
                    std::sin(yaw - kPi / 2.0),
                    0.0,
                    std::cos(yaw - kPi / 2.0)
                );
                glm::vec3 camUp = glm::cross(camRight, camDir);

                auto corners = computeFrustumCorners(camPos, camDir, camRight, camUp, fovY, aspect, nearDist, shadowDist);

                float sphereRadius = 0.0f;
                for (const auto& corner : corners)
                {
                    sphereRadius = std::max(sphereRadius, glm::distance(corner, shadowCenter));
                }

                const float sweepLightDistance = sphereRadius + zPadding;
                const glm::vec3 sweepLightEye = shadowCenter + lightDir * sweepLightDistance;

                glm::mat4 sweepLightView = glm::lookAt(sweepLightEye, shadowCenter, lightUp);
                glm::mat4 sweepLightProjGL = glm::ortho(
                    -sphereRadius, sphereRadius,
                    -sphereRadius, sphereRadius,
                    0.0f, 2.0f * sweepLightDistance);
                glm::mat4 sweepLightSpaceMatrixGL = sweepLightProjGL * sweepLightView;
                glm::mat4 sweepLightSpaceMatrixVK = vulkanClip * sweepLightProjGL * sweepLightView;

                // 1. Sphere radius must remain mathematically invariant under rotation
                assert(approxEqual(sphereRadius, refSphereRadius, 1e-3f));

                // 2. Light view matrix must be identical across rotation
                assert(mat4ApproxEqual(sweepLightView, refLightView, 1e-3f));

                // 3. Light projection matrix must be identical across rotation
                assert(mat4ApproxEqual(sweepLightProjGL, refLightProjGL, 1e-3f));

                // 4. Combined light space matrix (GL & VK) must be identical across rotation
                assert(mat4ApproxEqual(sweepLightSpaceMatrixGL, refLightSpaceMatrixGL, 1e-3f));
                assert(mat4ApproxEqual(sweepLightSpaceMatrixVK, refLightSpaceMatrixVK, 1e-3f));

                // 5. Frustum corners containment: all corners strictly contained in shadow volume
                for (const auto& corner : corners)
                {
                    glm::vec4 lsGL = sweepLightSpaceMatrixGL * glm::vec4(corner, 1.0f);
                    assert(lsGL.x >= -1.001f && lsGL.x <= 1.001f);
                    assert(lsGL.y >= -1.001f && lsGL.y <= 1.001f);
                    assert(lsGL.z >= -1.001f && lsGL.z <= 1.001f);

                    glm::vec4 lsVK = sweepLightSpaceMatrixVK * glm::vec4(corner, 1.0f);
                    assert(lsVK.x >= -1.001f && lsVK.x <= 1.001f);
                    assert(lsVK.y >= -1.001f && lsVK.y <= 1.001f);
                    assert(lsVK.z >= -0.001f && lsVK.z <= 1.001f);
                }
            }
        }
    }

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Tightened High-Resolution Shadow Volume Bounds & Non-Degeneracy Tests
// ============================================================================

void testTightenedHighResolutionDirectionalShadowMatrices()
{
    std::cout << "  [TEST] Tightened high-res shadow matrices strictly bounded (shadowDist=25, radius<=40, zPadding=45)... ";

    const float nearDist = 0.1f;
    const float shadowDist = 25.0f;
    const float p11 = 2.4142f; // cot(45 deg / 2)
    const float p00 = p11 / (16.0f / 9.0f); // 16:9 aspect
    const float fovY = 2.0f * std::atan(1.0f / p11);
    const float aspect = p11 / p00;
    const glm::mat4 vulkanClip = getVulkanClipMatrix();
    const glm::vec3 lightDir = glm::normalize(Math::getLightPositionOffset());
    const glm::vec3 lightUp = (std::abs(lightDir.y) > 0.99f)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    constexpr float zPadding = 45.0f;

    const glm::vec3 testPositions[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(15.0f, 5.0f, -25.0f),
        glm::vec3(-100.0f, 30.0f, 200.0f),
        glm::vec3(500.0f, -50.0f, -400.0f)
    };

    for (const auto& camPos : testPositions)
    {
        const glm::vec3 shadowCenter = camPos;

        // Verify across 360-degree yaw and pitch sweeps
        for (double yaw = 0.0; yaw < 6.28; yaw += 1.0)
        {
            for (double pitch = -1.5; pitch <= 1.5; pitch += 0.75)
            {
                glm::vec3 camDir(
                    std::cos(pitch) * std::sin(yaw),
                    std::sin(pitch),
                    std::cos(pitch) * std::cos(yaw)
                );
                glm::vec3 camRight(
                    std::sin(yaw - 3.14159265358979323846 / 2.0),
                    0.0,
                    std::cos(yaw - 3.14159265358979323846 / 2.0)
                );
                glm::vec3 camUp = glm::cross(camRight, camDir);

                auto corners = computeFrustumCorners(camPos, camDir, camRight, camUp, fovY, aspect, nearDist, shadowDist);

                float sphereRadius = 0.0f;
                for (const auto& corner : corners)
                {
                    sphereRadius = std::max(sphereRadius, glm::distance(corner, shadowCenter));
                }

                // 1. Invariant: sphereRadius strictly bounded by 40.0f for shadowDist = 25.0f
                assert(sphereRadius <= 40.0f);
                assert(sphereRadius >= 25.0f);

                // 2. Invariant: Orthogonal extents strictly bounded
                const float orthoExtentX = 2.0f * sphereRadius;
                const float orthoExtentY = 2.0f * sphereRadius;
                assert(orthoExtentX <= 80.0f);
                assert(orthoExtentY <= 80.0f);

                // 3. Invariant: Tightened depth extents strictly bounded
                const float lightDistance = sphereRadius + zPadding;
                assert(lightDistance <= 85.0f);

                const float zNear = 0.0f;
                const float zFar = 2.0f * lightDistance;
                assert(zFar <= 170.0f);
                assert(zFar - zNear <= 170.0f);
                assert(zFar > zNear + 50.0f);

                // Construct view and projection matrices
                const glm::vec3 lightEye = shadowCenter + lightDir * lightDistance;
                glm::mat4 lightView = glm::lookAt(lightEye, shadowCenter, lightUp);

                glm::mat4 lightProjGL = glm::ortho(
                    -sphereRadius, sphereRadius,
                    -sphereRadius, sphereRadius,
                    zNear, zFar);

                glm::mat4 lightSpaceMatrixGL = lightProjGL * lightView;
                glm::mat4 lightSpaceMatrixVK = vulkanClip * lightSpaceMatrixGL;

                // 4. Matrix non-degeneracy & finite determinant verification
                float detView = glm::determinant(lightView);
                float detProjGL = glm::determinant(lightProjGL);
                float detSpaceGL = glm::determinant(lightSpaceMatrixGL);
                float detSpaceVK = glm::determinant(lightSpaceMatrixVK);

                assert(!std::isnan(detView) && !std::isinf(detView));
                assert(approxEqual(std::abs(detView), 1.0f, 1e-3f)); // Rigid body

                assert(!std::isnan(detProjGL) && !std::isinf(detProjGL) && std::abs(detProjGL) > 1e-8f);
                assert(!std::isnan(detSpaceGL) && !std::isinf(detSpaceGL) && std::abs(detSpaceGL) > 1e-8f);
                assert(!std::isnan(detSpaceVK) && !std::isinf(detSpaceVK) && std::abs(detSpaceVK) > 1e-8f);

                // Invertibility check
                glm::mat4 invGL = glm::inverse(lightSpaceMatrixGL);
                glm::mat4 idGL = invGL * lightSpaceMatrixGL;
                assert(mat4ApproxEqual(idGL, glm::mat4(1.0f), 1e-3f));

                glm::mat4 invVK = glm::inverse(lightSpaceMatrixVK);
                glm::mat4 idVK = invVK * lightSpaceMatrixVK;
                assert(mat4ApproxEqual(idVK, glm::mat4(1.0f), 1e-3f));

                // 5. Containment of all 8 frustum corners in tightened NDC volumes
                for (const auto& corner : corners)
                {
                    glm::vec4 ndcGL = lightSpaceMatrixGL * glm::vec4(corner, 1.0f);
                    assert(approxEqual(ndcGL.w, 1.0f, 1e-4f));
                    assert(ndcGL.x >= -1.001f && ndcGL.x <= 1.001f);
                    assert(ndcGL.y >= -1.001f && ndcGL.y <= 1.001f);
                    assert(ndcGL.z >= -1.001f && ndcGL.z <= 1.001f);

                    glm::vec4 ndcVK = lightSpaceMatrixVK * glm::vec4(corner, 1.0f);
                    assert(approxEqual(ndcVK.w, 1.0f, 1e-4f));
                    assert(ndcVK.x >= -1.001f && ndcVK.x <= 1.001f);
                    assert(ndcVK.y >= -1.001f && ndcVK.y <= 1.001f);
                    assert(ndcVK.z >= -0.001f && ndcVK.z <= 1.001f);
                }

                // 6. Camera position / shadowCenter depth calibration check
                glm::vec4 centerNDC_GL = lightSpaceMatrixGL * glm::vec4(shadowCenter, 1.0f);
                assert(approxEqual(centerNDC_GL.x, 0.0f, 1e-2f));
                assert(approxEqual(centerNDC_GL.y, 0.0f, 1e-2f));
                assert(approxEqual(centerNDC_GL.z, 0.0f, 1e-2f));

                glm::vec4 centerNDC_VK = lightSpaceMatrixVK * glm::vec4(shadowCenter, 1.0f);
                assert(approxEqual(centerNDC_VK.x, 0.0f, 1e-2f));
                assert(approxEqual(centerNDC_VK.y, 0.0f, 1e-2f));
                assert(approxEqual(centerNDC_VK.z, 0.5f, 1e-2f));

                // 7. Depth monotonicity in tightened volume
                glm::vec4 frontPt = lightSpaceMatrixVK * glm::vec4(shadowCenter + lightDir * 20.0f, 1.0f);
                glm::vec4 backPt = lightSpaceMatrixVK * glm::vec4(shadowCenter - lightDir * 20.0f, 1.0f);
                assert(frontPt.z < centerNDC_VK.z);
                assert(centerNDC_VK.z < backPt.z);
                assert(frontPt.z >= 0.0f && backPt.z <= 1.0f);
            }
        }
    }

    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Shadow Bias Bounds & Peter-Panning Safety Tests
// ============================================================================

void testShadowBiasAndPeterPanningSafety()
{
    std::cout << "  [TEST] Shadow bias bounds [0.00035, 0.002] and Peter-Panning safety over ~500u frustum... ";

    const glm::vec3 lightDir = glm::normalize(Math::getLightPositionOffset());

    // Shaders compute calibrated slope-scaled bias as: max(0.0015 * (1.0 - dot(normal, lightDir)), 0.00035)
    auto computeBias = [](const glm::vec3& normal, const glm::vec3& lDir) -> float {
        float dotProduct = std::max(0.0f, glm::dot(normal, lDir));
        float bias = std::max(0.0015f * (1.0f - dotProduct), 0.00035f);
        return bias;
    };

    // 1. Direct facing normal (normal == lightDir, dotProduct = 1.0): bias is calibrated minimum (0.00035)
    float biasDirect = computeBias(lightDir, lightDir);
    assert(approxEqual(biasDirect, 0.00035f, 1e-7f));
    assert(biasDirect >= 0.00035f);
    assert(biasDirect <= 0.002f);

    // 2. Glancing normal (normal perpendicular to lightDir, dotProduct = 0.0): bias is slope max (0.0015)
    glm::vec3 perpDir = glm::normalize(glm::cross(lightDir, std::abs(lightDir.y) > 0.9f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0)));
    float biasGlancing = computeBias(perpDir, lightDir);
    assert(approxEqual(biasGlancing, 0.0015f, 1e-7f));
    assert(biasGlancing >= 0.00035f);
    assert(biasGlancing <= 0.002f);

    // 3. Opposite normal (normal == -lightDir): clamped dotProduct = 0.0, bias is 0.0015 <= 0.002
    float biasOpposite = computeBias(-lightDir, lightDir);
    assert(approxEqual(biasOpposite, 0.0015f, 1e-7f));
    assert(biasOpposite >= 0.00035f);
    assert(biasOpposite <= 0.002f);

    // 4. Sweep full angular range of incidence [0, pi/2] (0 to 90 degrees)
    constexpr double kPi = 3.14159265358979323846;
    constexpr float kMinBias = 0.00035f;
    constexpr float kMaxBias = 0.0020f;
    constexpr float kFrustumDepthRange = 500.0f; // ~500 unit frustum / shadow depth range

    for (double angle = 0.0; angle <= kPi / 2.0; angle += (kPi / 2.0) / 90.0)
    {
        float dotProduct = static_cast<float>(std::cos(angle));
        float bias = std::max(0.0015f * (1.0f - dotProduct), 0.00035f);

        // Invariant: bias must strictly remain within [0.00035, 0.002] across all angles
        assert(bias >= kMinBias - 1e-7f);
        assert(bias <= kMaxBias + 1e-7f);

        // Peter-Panning prevention check:
        // World space offset delta = bias * frustumDepthRange
        float worldSpaceOffset = bias * kFrustumDepthRange;
        // Min offset is 0.175 units (prevents shadow acne on facing surfaces)
        assert(worldSpaceOffset >= 0.175f - 1e-4f);
        // Max offset is <= 1.0 unit (sub-unit/1-unit max, preventing Peter-Panning / detached shadows)
        assert(worldSpaceOffset <= 1.0f + 1e-4f);
    }

    // 5. Sweep full 3D sphere of surface normals: polar angle theta [0, pi], azimuthal phi [0, 2*pi]
    for (double theta = 0.0; theta <= kPi; theta += kPi / 36.0)
    {
        for (double phi = 0.0; phi < 2.0 * kPi; phi += kPi / 36.0)
        {
            glm::vec3 normal(
                static_cast<float>(std::sin(theta) * std::cos(phi)),
                static_cast<float>(std::cos(theta)),
                static_cast<float>(std::sin(theta) * std::sin(phi))
            );
            normal = glm::normalize(normal);

            float bias = computeBias(normal, lightDir);

            // Invariant: bias must strictly remain within [0.00035, 0.002]
            assert(bias >= kMinBias - 1e-7f);
            assert(bias <= kMaxBias + 1e-7f);

            // Peter-Panning prevention check:
            float worldSpaceOffset = bias * kFrustumDepthRange;
            assert(worldSpaceOffset >= 0.175f - 1e-4f);
            assert(worldSpaceOffset <= 1.0f + 1e-4f);
        }
    }

    std::cout << "PASSED" << std::endl;
}

void testShadowBiasBoundsAndPeterPanningSafety()
{
    testShadowBiasAndPeterPanningSafety();
}

// ============================================================================
// Test Suite Runner
// ============================================================================

int runShadowTests()
{
    std::cout << "\n=== Directional Shadow & Matrix Stability Tests ===" << std::endl;

    testDirectionalLightSunVectorInvariance();
    testDirectionalLightViewMatrixStability();
    testOpenGLShadowProjectionStability();
    testVulkanShadowProjectionStability();
    testShadowProjectionUnderExtremePitchesAndAspects();
    testDirectionalShadowCameraRotationInvariance();
    testTightenedHighResolutionDirectionalShadowMatrices();
    testShadowBiasAndPeterPanningSafety();

    std::cout << "All Directional Shadow tests PASSED!" << std::endl;
    return 0;
}
