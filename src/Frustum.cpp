#include "Frustum.h"

static constexpr int PLANE_XNEG = 0;  // left
static constexpr int PLANE_XPOS = 1;  // right
static constexpr int PLANE_YNEG = 2;  // bottom
static constexpr int PLANE_YPOS = 3;  // top
static constexpr int PLANE_ZNEG = 4;  // near
static constexpr int PLANE_ZPOS = 5;  // far

void Frustum::normalizePlane(std::array<float, 4>& plane)
{
    const float len = static_cast<float>(std::sqrt(
        plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]
    ));
    if (len > 1e-6f) {
        plane[0] /= len;
        plane[1] /= len;
        plane[2] /= len;
        plane[3] /= len;
    }
}

void Frustum::extractFrustum(const glm::mat4& viewProjectionMatrix)
{
    // GLM stores matrices in column-major order, but the frustum extraction
    // formulas are defined for row vectors. Transposing once makes the
    // standard OpenGL plane equations align with the matrix layout we use.
    const glm::mat4 m = glm::transpose(viewProjectionMatrix);

    // Left plane
    planes[PLANE_XNEG][0] = m[3][0] + m[0][0];
    planes[PLANE_XNEG][1] = m[3][1] + m[0][1];
    planes[PLANE_XNEG][2] = m[3][2] + m[0][2];
    planes[PLANE_XNEG][3] = m[3][3] + m[0][3];
    normalizePlane(planes[PLANE_XNEG]);

    // Right plane
    planes[PLANE_XPOS][0] = m[3][0] - m[0][0];
    planes[PLANE_XPOS][1] = m[3][1] - m[0][1];
    planes[PLANE_XPOS][2] = m[3][2] - m[0][2];
    planes[PLANE_XPOS][3] = m[3][3] - m[0][3];
    normalizePlane(planes[PLANE_XPOS]);

    // Bottom plane
    planes[PLANE_YNEG][0] = m[3][0] + m[1][0];
    planes[PLANE_YNEG][1] = m[3][1] + m[1][1];
    planes[PLANE_YNEG][2] = m[3][2] + m[1][2];
    planes[PLANE_YNEG][3] = m[3][3] + m[1][3];
    normalizePlane(planes[PLANE_YNEG]);

    // Top plane
    planes[PLANE_YPOS][0] = m[3][0] - m[1][0];
    planes[PLANE_YPOS][1] = m[3][1] - m[1][1];
    planes[PLANE_YPOS][2] = m[3][2] - m[1][2];
    planes[PLANE_YPOS][3] = m[3][3] - m[1][3];
    normalizePlane(planes[PLANE_YPOS]);

    // Near plane
    planes[PLANE_ZNEG][0] = m[3][0] + m[2][0];
    planes[PLANE_ZNEG][1] = m[3][1] + m[2][1];
    planes[PLANE_ZNEG][2] = m[3][2] + m[2][2];
    planes[PLANE_ZNEG][3] = m[3][3] + m[2][3];
    normalizePlane(planes[PLANE_ZNEG]);

    // Far plane
    planes[PLANE_ZPOS][0] = m[3][0] - m[2][0];
    planes[PLANE_ZPOS][1] = m[3][1] - m[2][1];
    planes[PLANE_ZPOS][2] = m[3][2] - m[2][2];
    planes[PLANE_ZPOS][3] = m[3][3] - m[2][3];
    normalizePlane(planes[PLANE_ZPOS]);
}

[[nodiscard]] bool Frustum::pointInFrustum(float x, float y, float z) const
{
    for (int i = 0; i < 6; ++i) {
        if (planes[i][0] * x + planes[i][1] * y + planes[i][2] * z + planes[i][3] <= 0.0f) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool Frustum::sphereInFrustum(float x, float y, float z, float radius) const
{
    for (int i = 0; i < 6; ++i) {
        if (planes[i][0] * x + planes[i][1] * y + planes[i][2] * z + planes[i][3] <= -radius) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] float Frustum::sphereInFrustumDistance(float x, float y, float z, float radius) const
{
    float minDist = 0.0f;
    for (int i = 0; i < 6; ++i) {
        const float d = planes[i][0] * x + planes[i][1] * y + planes[i][2] * z + planes[i][3];
        if (d <= -radius) {
            return 0.0f;
        }
        minDist = (d > minDist) ? d : minDist;
    }
    return minDist + radius;
}

[[nodiscard]] int Frustum::spherePartiallyInFrustum(float x, float y, float z, float radius) const
{
    int fullyInsidePlanes = 0;
    for (int i = 0; i < 6; ++i) {
        const float d = planes[i][0] * x + planes[i][1] * y + planes[i][2] * z + planes[i][3];
        if (d <= -radius) {
            return 0; // Completely outside
        }
        if (d > radius) {
            ++fullyInsidePlanes;
        }
    }
    return (fullyInsidePlanes == 6) ? 2 : 1; // 2 = fully inside, 1 = partially inside
}

[[nodiscard]] bool Frustum::cubeInFrustum(float x, float y, float z, float size) const
{
    // Check if all 8 corners of the cube are outside any plane
    const float corners[8][3] = {
        {-size, -size, -size}, { size, -size, -size}, { size,  size, -size}, {-size,  size, -size},
        {-size, -size,  size}, { size, -size,  size}, { size,  size,  size}, {-size,  size,  size}
    };

    for (int i = 0; i < 6; ++i) {
        int insideCount = 0;
        for (int j = 0; j < 8; ++j) {
            const float px = x + corners[j][0];
            const float py = y + corners[j][1];
            const float pz = z + corners[j][2];
            if (planes[i][0] * px + planes[i][1] * py + planes[i][2] * pz + planes[i][3] > 0.0f) {
                ++insideCount;
            }
        }
        if (insideCount == 0) {
            return false; // All corners outside this plane
        }
    }
    return true;
}

[[nodiscard]] int Frustum::cubePartiallyInFrustum(float x, float y, float z, float size) const
{
    const float corners[8][3] = {
        {-size, -size, -size}, { size, -size, -size}, { size,  size, -size}, {-size,  size, -size},
        {-size, -size,  size}, { size, -size,  size}, { size,  size,  size}, {-size,  size,  size}
    };

    int fullyInsidePlanes = 0;
    for (int i = 0; i < 6; ++i) {
        int insideCount = 0;
        for (int j = 0; j < 8; ++j) {
            const float px = x + corners[j][0];
            const float py = y + corners[j][1];
            const float pz = z + corners[j][2];
            if (planes[i][0] * px + planes[i][1] * py + planes[i][2] * pz + planes[i][3] > 0.0f) {
                ++insideCount;
            }
        }
        if (insideCount == 0) return 0; // Completely outside
        if (insideCount == 8) ++fullyInsidePlanes;
    }
    return (fullyInsidePlanes == 6) ? 2 : 1;
}

[[nodiscard]] bool Frustum::polygonInFrustum(int numpoints, const glm::vec3* pointlist) const
{
    for (int f = 0; f < 6; ++f) {
        int insideCount = 0;
        for (int p = 0; p < numpoints; ++p) {
            if (planes[f][0] * pointlist[p].x + planes[f][1] * pointlist[p].y + planes[f][2] * pointlist[p].z + planes[f][3] > 0.0f) {
                ++insideCount;
            }
        }
        if (insideCount == 0) {
            return false; // All points outside this plane
        }
    }
    return true;
}
