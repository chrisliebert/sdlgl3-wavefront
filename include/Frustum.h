#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include "Common.h"

typedef struct
{
    float x, y, z;
} Point;

class Frustum
{
public:
    void extractFrustum(glm::mat4& modelViewMatrix, glm::mat4& projectionMatrix);
    bool pointInFrustum( float x, float y, float z );
    bool sphereInFrustum( float x, float y, float z, float radius );
    float sphereInFrustumDistance( float x, float y, float z, float radius );
    int spherePartiallyInFrustum( float x, float y, float z, float radius );
    bool cubeInFrustum( float x, float y, float z, float size );
    int cubePartiallyInFrustum( float x, float y, float z, float size );
    bool polygonInFrustum(int numpoints, Point* pointlist);

private:
    float frustum[6][4];
};

#endif // _FRUSTUM_H_
