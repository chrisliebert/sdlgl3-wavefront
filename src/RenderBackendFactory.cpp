#include "RenderBackendFactory.h"
#include "OpenGLBackend.h"
#include "SdlGpuBackend.h"
#include "VulkanBackend.h"
#include "Renderer.h"
#include <SDL3/SDL.h>

std::unique_ptr<IRenderBackend> RenderBackendFactory::createBackend(
    Renderer& renderer,
    const ConfigLoader& config,
    SDL_Window*& window)
{
    RenderBackendType backendType = config.getRenderBackend();
    std::unique_ptr<IRenderBackend> backend = nullptr;

    if (backendType == RenderBackendType::AUTO || backendType == RenderBackendType::OPENGL) {
        window = SDL_CreateWindow("sdlgl3-wavefront",
            config.hasVar("window.width") ? config.getInt("window.width") : 1280, config.hasVar("window.height") ? config.getInt("window.height") : 720,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        
        backend = std::make_unique<OpenGLBackend>(renderer);
    }
    else if (backendType == RenderBackendType::VULKAN) {
        window = SDL_CreateWindow("sdlgl3-wavefront",
            config.hasVar("window.width") ? config.getInt("window.width") : 1280, config.hasVar("window.height") ? config.getInt("window.height") : 720,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        
        backend = std::make_unique<VulkanBackend>(renderer);
    }
    return backend;
}
