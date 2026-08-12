#ifndef _RENDER_BACKEND_H_
#define _RENDER_BACKEND_H_

#include "Common.h"
#include "Camera.h"
#include "SceneNode.h"
#include <vector>
#include <memory>
#include <string_view>

struct RenderCommand {
    enum class CommandType : int {
        DRAW_ELEMENTS = 0,
        DRAW_ARRAYS = 1,
        CLEAR = 2,
        SET_PIPELINE = 3,
        SET_UNIFORM_BUFFER = 4
    };

    CommandType type;
    const SceneNode* node = nullptr;
    
    RenderCommand() : type(CommandType::DRAW_ELEMENTS), node(nullptr) {}
    explicit RenderCommand(CommandType t, const SceneNode* n) : type(t), node(n) {}
};

/**
 * @class IRenderBackend
 * @brief Abstract interface for rendering backends (OpenGL, SDL_gpu, Vulkan, etc.)
 * 
 * This interface defines the complete set of operations needed for rendering,
 * including GPU buffer management and shadow mapping. Each backend implements
 * these operations using its target graphics API.
 */
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Frame lifecycle
    virtual bool initialize(SDL_Window* window) = 0;
    virtual void shutdown() = 0;
    virtual void beginFrame(const FrameContext& frameContext) = 0;
    virtual void submit(const std::vector<RenderCommand>& commands) = 0;
    virtual void endFrame() = 0;

    // GPU buffer management (moved from Renderer to backend)
    virtual bool bufferToGpu(Camera& camera, std::string_view cacheFilename, bool loadCachedScene) = 0;
    
    // Shadow mapping
    virtual GLuint createShadowMap(Camera& camera) = 0;
    virtual void updateShadowMap(Camera& camera) = 0;
    
    // Update per-frame camera and light uniforms for the current frame.
    virtual void updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos) = 0;
    
    // Query current shadow map dimensions
    virtual void getShadowMapSize(int& width, int& height) const = 0;

    virtual void fitDirectionalShadowMatrix(Camera& camera, const glm::vec3& lightPos, int shadowWidth, int shadowHeight, glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix) = 0;

    // Screenshot capability (optional)
    virtual void takeScreenshot(std::string_view filename) { (void)filename; }
};

#endif // _RENDER_BACKEND_H_
