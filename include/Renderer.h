#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Camera.h"
#include "Common.h"
#include "Frustum.h"
#include "GpuProgram.h"
#include "Material.h"
#include "SceneNode.h"

#include <SDL_image.h>

void _checkForGLError(const char *file, int line);
// Usage
// [some opengl calls]
// glCheckError();
#define checkForGLError() _checkForGLError(__FILE__,__LINE__)

class Renderer
{
public:
    Renderer();
    ~Renderer();
    void addMaterial(Material*);
    void addSceneNode(SceneNode*);
    void addTexture(const char*, GLuint&);
    void addWavefront(const char*, glm::mat4);
    void buildScene();
    void render(Camera*);
private:
    GLuint vao, vbo, ibo;
    GLuint startPosition;
    glm::mat4 modelViewProjectionMatrix;
    GpuProgram *gpuProgram;
    std::vector<Vertex> vertexData;
    Frustum frustum;
    std::vector<SceneNode> sceneNodes;
    std::vector<GLuint> indices;
    std::map<std::string, Material> materials;
    std::map<std::string, SDL_Surface*> textures;
};

#endif
