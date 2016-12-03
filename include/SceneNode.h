#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

#include "Common.h"
#include "Material.h"
#include "GpuProgram.h"

/*
typedef struct {
    float top, bottom, left, right, front, back;
} BoundingBox;
*/

typedef struct {
	char name[MAX_NODE_NAME_STRING_LENGTH];
    char material[MAX_MATERIAL_NAME_STRING_LENGTH];
	Vertex* vertexData;
	size_t vertexDataSize;
    glm::mat4 modelViewMatrix;
    GLuint startPosition;
    GLuint endPosition;
    GLenum primativeMode;

    GLuint ambientTextureId;
    GLuint diffuseTextureId;
    GLuint normalTextureId;
    GLuint specularTextureId;

    GLfloat boundingSphere;
    GLfloat lx, ly, lz;
} SceneNode;

//BoundingBox* getBoundingBox(SceneNode*);

#endif
