#include "RenderBackendFactory.h"
#include "OpenGLBackend.h"
#include "SdlGpuBackend.h"
#include "VulkanBackend.h"
#include "Renderer.h"
#include <SDL3/SDL.h>
#include <iostream>

namespace {
const char* backendName(RenderBackendType type)
{
    switch (type)
    {
    case RenderBackendType::AUTO: return "auto";
    case RenderBackendType::OPENGL: return "opengl";
    case RenderBackendType::VULKAN: return "vulkan";
    default: return "unknown";
    }
}
}

std::unique_ptr<IRenderBackend> RenderBackendFactory::createBackend(
    Renderer& renderer,
    const ConfigLoader& config,
    SDL_Window*& window)
{
    RenderBackendType backendType = config.getRenderBackend();
    std::unique_ptr<IRenderBackend> backend = nullptr;

    std::cout << "Backend factory: requested backend = " << backendName(backendType) << std::endl;

    if (backendType == RenderBackendType::AUTO || backendType == RenderBackendType::OPENGL) {
        window = SDL_CreateWindow("sdlgl3-wavefront",
            config.hasVar("window.width") ? config.getInt("window.width") : 1280, config.hasVar("window.height") ? config.getInt("window.height") : 720,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
        {
            std::cerr << "Backend factory: SDL_CreateWindow(OpenGL) failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        backend = std::make_unique<OpenGLBackend>(renderer);
    }
    else if (backendType == RenderBackendType::VULKAN) {
        window = SDL_CreateWindow("sdlgl3-wavefront",
            config.hasVar("window.width") ? config.getInt("window.width") : 1280, config.hasVar("window.height") ? config.getInt("window.height") : 720,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (window == nullptr)
        {
            std::cerr << "Backend factory: SDL_CreateWindow(Vulkan) failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        
        backend = std::make_unique<VulkanBackend>(renderer);
    }

    std::cout << "Backend factory: backend object " << (backend ? "created" : "not created") << std::endl;
    return backend;
}
