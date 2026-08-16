#pragma once

#include "Common.h"
#include <memory>
#include <vector>

// Forward declarations
class SceneNode;
class ConfigLoader;
struct SDL_Window;

// An interface for a rendering backend (e.g., OpenGL, Vulkan)
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Initialization and shutdown
    virtual bool initialize(SDL_Window* window, ConfigLoader& configLoader) = 0;
    virtual void shutdown() = 0;

    // Main rendering call
    virtual void render(const std::vector<SceneNode*>& nodes) = 0;

    // Resource management
    virtual bool bufferToGpu(const std::vector<SceneNode*>& nodes) = 0;
    virtual void createShadowMap() = 0;

    // UI Rendering
    virtual void initUIRendering() = 0;
    virtual void shutdownUIRendering() = 0;
    virtual void beginUIRender() = 0;
    virtual void endUIRender() = 0;
};

// A factory for creating a render backend
std::unique_ptr<IRenderBackend> createBackend(SDL_Window* window, ConfigLoader& configLoader);
