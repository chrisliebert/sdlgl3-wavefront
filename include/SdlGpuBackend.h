#ifndef _SDLGPU_BACKEND_H_
#define _SDLGPU_BACKEND_H_

#include "RenderBackend.h"
#include <glm/glm.hpp>

#if __has_include(<SDL3/SDL_gpu.h>)
#include <SDL3/SDL_gpu.h>
#elif __has_include(<SDL_gpu.h>)
#include <SDL_gpu.h>
#else
#error "SDL_gpu.h not found"
#endif

class Renderer; // Forward declaration

class SdlGpuBackend : public IRenderBackend {
public:
    SdlGpuBackend(Renderer& renderer);
    ~SdlGpuBackend() override;

    bool initialize(SDL_Window* window) override;
    void shutdown() override;
    void beginFrame(const FrameContext& frameContext) override;
    void submit(const std::vector<RenderCommand>& commands) override;
    void endFrame() override;

    // SDL_gpu-specific method
    bool bufferToGpu(Camera& camera);
    void takeScreenshot(const char* filename);
    
    // IRenderBackend shadow interface (stub for SDL_gpu backend)
    GLuint createShadowMap(Camera& camera) override { (void)camera; return 0; }
    void updateShadowMap(Camera& camera) override { (void)camera; }
    void getShadowMapSize(int& width, int& height) const override { width = 0; height = 0; }
    void updateLightUniforms(const Camera&, const glm::mat4&, const glm::vec3&) override {}
    void takeScreenshot(std::string_view filename) override { (void)filename; }

private:
    Renderer& owner;
    SDL_Window* window;
    SDL_GpuDevice* device;

    SDL_GpuBuffer* vbo;
    SDL_GpuBuffer* ibo;
    SDL_GpuGraphicsPipeline* pipeline;
    SDL_GpuBuffer* ubo;
    SDL_GpuTexture* depthTexture;
    uint32_t swapchainWidth;
    uint32_t swapchainHeight;
};

#endif // _SDLGPU_BACKEND_H_