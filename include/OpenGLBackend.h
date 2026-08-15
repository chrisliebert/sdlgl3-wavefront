#ifndef _OPENGL_BACKEND_H_
#define _OPENGL_BACKEND_H_

#include "RenderBackend.h"
#include "GpuProgram.h"
#include <memory>
#include <string_view>
#include <vector>
#include <array>
#include <string>

class Renderer;
class ConfigLoader;

/**
 * @class OpenGLBackend
 * @brief OpenGL 3.3+ implementation of IRenderBackend
 * 
 * Handles all OpenGL-specific rendering operations including:
 * - Vertex buffer management with persistent mapping support
 * - Shader program compilation and management
 * - Shadow map generation with directional light frustum fitting
 * - Framebuffer object management for shadow pass
 */
class OpenGLBackend : public IRenderBackend {
public:
    explicit OpenGLBackend(Renderer& renderer);
    ~OpenGLBackend() override;

    // IRenderBackend interface implementation
    bool initialize(SDL_Window* window) override;
    void shutdown() override;
    void beginFrame(const FrameContext& frameContext) override;
    void submit(const std::vector<RenderCommand>& commands) override;
    void endFrame() override;
    
    // GPU buffer management
    bool bufferToGpu(Camera& camera, std::string_view cacheFilename, bool loadCachedScene) override;
    
    // Shadow mapping
    GLuint createShadowMap(Camera& camera) override;
    void updateShadowMap(Camera& camera) override;
    void getShadowMapSize(int& width, int& height) const override {
        width = shadowWidthW;
        height = shadowHeightH;
    }

    void fitDirectionalShadowMatrix(Camera& camera, const glm::vec3& lightPos, int shadowWidth, int shadowHeight, glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix) override;

    // IRenderBackend updateLightUniforms implementation
    void updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos) override;

private:
    Renderer& owner;
    SDL_Window* window = nullptr;

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

    // Helper to get config
    ConfigLoader& getConfig();
};

#endif // _OPENGL_BACKEND_H_
