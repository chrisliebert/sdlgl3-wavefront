#include "SceneNode.h"

BoundingBox* getBoundingBox(SceneNode* mesh)
{
    BoundingBox *bbox = new BoundingBox;
    if(mesh  == 0) {
        std::cerr << "Unable to get bounding box of null mesh" << std::endl;
    }

    bbox->top = bbox->bottom = bbox->left = bbox->right = bbox->front = bbox->back = 0.f;
    for(int i=0; i<mesh->vertexDataSize; i++) {
        if      (mesh->vertexData[i].vertex[0] < bbox->left)      { bbox->left     = mesh->vertexData[i].vertex[0]; }
        else if (mesh->vertexData[i].vertex[0] > bbox->right)     { bbox->right    = mesh->vertexData[i].vertex[0]; }
        if      (mesh->vertexData[i].vertex[1] < bbox->bottom)    { bbox->left     = mesh->vertexData[i].vertex[1]; }
        else if (mesh->vertexData[i].vertex[1] > bbox->top)       { bbox->right    = mesh->vertexData[i].vertex[1]; }
        if      (mesh->vertexData[i].vertex[2] < bbox->back)      { bbox->back     = mesh->vertexData[i].vertex[2]; }
        else if (mesh->vertexData[i].vertex[2] > bbox->front)     { bbox->front    = mesh->vertexData[i].vertex[2]; }
    }
    return bbox;
}
