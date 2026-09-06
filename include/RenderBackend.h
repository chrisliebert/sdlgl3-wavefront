#pragma once

#include "Common.h"
#include <memory>
#include <vector>

// Forward declarations
class Camera;
class ConfigLoader;
struct SDL_Window;
struct Vertex;
struct SceneNode;

namespace Capture
{
    struct Frame;
}

struct RenderCommand
{
    enum class CommandType
    {
        DRAW_ELEMENTS,
        // Future commands can be added here
    };

    CommandType type;
    const SceneNode* node = nullptr;
};

// An interface for a rendering backend (e.g., OpenGL, Vulkan)
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Initialization and shutdown
    virtual bool initialize(SDL_Window* window) = 0;
    virtual void shutdown() = 0;

    // Main rendering call
    virtual void submit(const std::vector<RenderCommand>& commands) = 0;

    // Resource management
    virtual bool bufferToGpu(const std::vector<Vertex>& vertexData, const std::vector<uint32_t>& indices) = 0;
    virtual void createShadowMap(const std::vector<SceneNode>& nodes) = 0;
    virtual void getShadowMapSize(int& width, int& height) const = 0;
    virtual void fitDirectionalShadowMatrix(Camera& camera, const glm::vec3& lightPosition, int shadowWidth, int shadowHeight, glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix) = 0;
    virtual void updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos) = 0;
    virtual void beginFrame(const struct FrameContext& frameContext) = 0;
    virtual void endFrame() = 0;

    // UI Rendering
    virtual void initUIRendering() = 0;
    virtual void shutdownUIRendering() = 0;
    virtual void beginUIRender() = 0;
    virtual void endUIRender() = 0;

    // Frame capture
    virtual std::unique_ptr<Capture::Frame> captureFrame() = 0;
};
