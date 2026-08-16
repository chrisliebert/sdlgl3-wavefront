#ifndef _OPENGL_BACKEND_H_
#define _OPENGL_BACKEND_H_

#include "RenderBackend.h"
#include "GpuProgram.h"
#include <memory>
#include <vector>

// Forward declarations
class SceneNode;
class ConfigLoader;
struct SDL_Window;

/**
 * @class OpenGLBackend
 * @brief OpenGL 3.3+ implementation of IRenderBackend
 */
class OpenGLBackend : public IRenderBackend {
public:
    OpenGLBackend();
    ~OpenGLBackend() override;

    // IRenderBackend interface implementation
    bool initialize(SDL_Window* window, ConfigLoader& configLoader) override;
    void shutdown() override;
    void render(const std::vector<SceneNode*>& nodes) override;
    bool bufferToGpu(const std::vector<SceneNode*>& nodes) override;
    void createShadowMap() override;

    // UI Rendering
    void initUIRendering() override;
    void shutdownUIRendering() override;
    void beginUIRender() override;
    void endUIRender() override;

private:
    SDL_Window* m_window = nullptr;
    ConfigLoader* m_configLoader = nullptr;

    // VBO configuration
    bool usePersistentMappedVbo = false;
    bool persistentVboActive = false;
    void* persistentVboPtr = nullptr;

    // OpenGL resource handles
    GLuint vao = 0, vbo = 0, ibo = 0;
    GLuint shadowMap = 0, depthMapFBO = 0;
    GLuint fallbackWhiteTexture = 0;
    GLuint fallbackNormalTexture = 0;
    std::unique_ptr<GpuProgram> gpuProgram;
    std::unique_ptr<GpuProgram> shadowProgram;
    int shadowWidthW = 2048;
    int shadowHeightH = 2048;
    bool verboseLogging = false;
    bool cullFaceEnabled = false;
    std::string depthVertShaderPath;
    std::string depthFragShaderPath;
    std::string mainVertShaderPath;
    std::string mainFragShaderPath;
    float clearR = 1.0f;
    float clearG = 0.8f;
    float clearB = 0.8f;
    float clearA = 1.0f;
};

#endif // _OPENGL_BACKEND_H_
