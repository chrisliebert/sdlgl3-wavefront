#pragma once

#include "Common.h"
#include "ConfigLoader.h"
#include "RenderBackend.h"
#include <memory>

struct SDL_Window;

class RenderBackendFactory
{
public:
    static std::unique_ptr<IRenderBackend> createBackend(
        class Renderer& renderer,
        const ConfigLoader& config,
        SDL_Window*& window
    );
};
