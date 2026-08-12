#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "Common.h"
#include <array>
#include <cmath>

// ============================================================================
// Frustum (viewing-volume culling via plane extraction)
// ============================================================================
using Point = glm::vec3;

class Frustum {
public:
    /** Extract frustum planes from a combined view-projection matrix.
     *  Each plane is stored as [nx, ny, nz, d] representing
     *  nx*x + ny*y + nz*z + d >= 0 (inside half-space). */
    void extractFrustum(const glm::mat4& viewProjectionMatrix);
    
    [[nodiscard]] bool pointInFrustum(float x, float y, float z) const;
    [[nodiscard]] bool sphereInFrustum(float x, float y, float z, float radius) const;
    [[nodiscard]] float sphereInFrustumDistance(float x, float y, float z, float radius) const;
    [[nodiscard]] int spherePartiallyInFrustum(float x, float y, float z, float radius) const;
    [[nodiscard]] bool cubeInFrustum(float x, float y, float z, float size) const;
    [[nodiscard]] int cubePartiallyInFrustum(float x, float y, float z, float size) const;

    /** Legacy polygon test using glm::vec3 as Point replacement. */
    [[nodiscard]] bool polygonInFrustum(int numpoints, const glm::vec3* pointlist) const;

private:
    std::array<std::array<float, 4>, 6> planes{};
    
    void normalizePlane(std::array<float, 4>& plane);
};

#endif // _FRUSTUM_H_
