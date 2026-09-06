#ifndef _OPENGL_BACKEND_H_
#define _OPENGL_BACKEND_H_

#include "RenderBackend.h"
#include "GpuProgram.h"
#include "Capture.h"
#include <memory>
#include <vector>

// Forward declarations
struct Camera;
struct SceneNode;
class ConfigLoader;
struct SDL_Window;

/**
 * @class OpenGLBackend
 * @brief OpenGL 3.3+ implementation of IRenderBackend
 */
class OpenGLBackend : public IRenderBackend {
public:
    explicit OpenGLBackend(class Renderer& renderer);
    ~OpenGLBackend() override;

    // IRenderBackend interface implementation
    bool initialize(SDL_Window* window) override;
    void shutdown() override;
    bool isOpenGL() const override { return true; }
    void submit(const std::vector<RenderCommand>& commands) override;
    void addTexture(GLuint* textureId, const struct Texture* texture) override;
    bool bufferToGpu(const std::vector<struct Vertex>& vertexData, const std::vector<uint32_t>& indices) override;
    void createShadowMap(const std::vector<struct SceneNode>& nodes) override;

    // UI Rendering
    void initUIRendering() override;
    void shutdownUIRendering() override;
    void beginUIRender() override;
    void endUIRender() override;

    // Frame capture
    std::unique_ptr<Capture::Frame> captureFrame() override;

    void fitDirectionalShadowMatrix(Camera& camera, const glm::vec3& lightPosition, int shadowWidth, int shadowHeight, glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix) override;
    void updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos) override;
    void getShadowMapSize(int& width, int& height) const override;
    void beginFrame(const struct FrameContext& frameContext) override;
    void endFrame() override;
    void drawCullDebugOverlay(const Camera& camera, const std::vector<int>& visibleNodeIds);


private:
    Renderer& owner;
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    ConfigLoader& getConfig();


    // VBO configuration
    bool usePersistentMappedVbo = false;
    bool persistentVboActive = false;
    void* persistentVboPtr = nullptr;

    // OpenGL resource handles
    GLuint vao = 0, vbo = 0, ibo = 0;
    size_t vboSize = 0;
    size_t iboSize = 0;
    GLuint shadowMap = 0, depthMapFBO = 0;
    GLuint fallbackWhiteTexture = 0;
    GLuint fallbackNormalTexture = 0;
    std::unique_ptr<GpuProgram> gpuProgram;
    std::unique_ptr<GpuProgram> shadowProgram;
    
    // Cached uniform indices
    size_t uniformProj = static_cast<size_t>(-1);
    size_t uniformView = static_cast<size_t>(-1);
    size_t uniformViewPos = static_cast<size_t>(-1);
    size_t uniformLightSpace = static_cast<size_t>(-1);
    size_t uniformLightPos = static_cast<size_t>(-1);
    size_t uniformHasNormal = static_cast<size_t>(-1);
    size_t uniformHasSpecular = static_cast<size_t>(-1);
    size_t shadowUniformLightSpace = static_cast<size_t>(-1);

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

    std::vector<RenderCommand> lastSubmittedCommands;
    GLuint m_prevVAO = 0;
    GLsync m_vboFence = nullptr;

    GLuint debugProgram = 0;
    GLuint debugVao = 0;
    GLuint debugVbo = 0;
    void ensureDebugProgram();
    void drawCullDebugOverlay();
    void drawDebugLines(const std::vector<glm::vec3>& linePoints, const glm::vec3& color, const glm::mat4& viewProjection) const;
};

#endif // _OPENGL_BACKEND_H_
