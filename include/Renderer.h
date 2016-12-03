#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Camera.h"
#include "Common.h"
#include "Frustum.h"
#include "GpuProgram.h"
#include "Material.h"
#include "SceneNode.h"

#include <SDL_image.h>
#include <SDL_thread.h>

void _checkForGLError(const char *file, int line);
// Usage
// [some opengl calls]
// glCheckError();
#define checkForGLError() _checkForGLError(__FILE__,__LINE__)

// GLAD_DEBUG is only defined if the c-debug generator was used
#ifdef GLAD_DEBUG
// logs every gl call to the console
void pre_gl_call(const char *name, void *funcptr, int len_args, ...);
#endif

// Load configuration variables
class ConfigLoader
{
protected:
	const char* filename;
	std::map<std::string, std::string> vars;
public:
	// Load a .cfg file
	ConfigLoader(const char*);
	~ConfigLoader();
	bool getBool(const char*);
	bool getBool(std::string&);
	int getInt(const char*);
	int getInt(std::string&);
	float getFloat(const char*);
	float getFloat(std::string&);
	std::string& getVar(const char*);
	std::string& getVar(std::string&);
	bool hasVar(const char*);
	bool hasVar(std::string&);
	friend std::ostream& operator<<(std::ostream& os, ConfigLoader& dt); // used for debugging
	friend std::ostream& operator<<(std::ostream& os, ConfigLoader* dt); // used for debugging
};

class Renderer
{
public:
    Renderer();
    ~Renderer();
    void addMaterial(Material*);
    void addSceneNode(SceneNode*);
    void addTexture(const char*, GLuint*, SDL_Surface*);
    void addTexture(const char*, GLuint*, Texture*);
    void addTexture(const char*, GLuint*);
    void addWavefront(const char*, glm::mat4);
    bool buildScene(Camera&, const char*); //TODO check if cam is needed
    bool buildScene(Camera&);

    void bufferToGpu(Camera&, bool);
    bool checkScene();
    GLuint createShadowMap(Camera&);
    void render(Camera*);
    void enableShadows();
    void disableShadows();
	std::vector<Vertex> vertexData;
    std::vector<SceneNode> sceneNodes;
    std::vector<GLuint> indices;
    std::map<std::string, Material> materials;
    std::map<std::string, Texture> textures;
    ConfigLoader* configLoader;
    std::string cacheFileName;
private:
    bool shadowsEnabled;
    GLuint vao, vbo, ibo;
    GLuint startPosition;
    GLuint shadowMap, depthMapFBO;
    glm::mat4 modelViewProjectionMatrix;
    GpuProgram *gpuProgram, *shadowProgram;
    Frustum frustum;
    int shadowWidth, shadowHeight;
    SDL_Thread *binCacheWriterThread;
};

#endif
