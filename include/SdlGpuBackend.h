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
    bool bufferToGpu(const std::vector<Vertex>& vertexData, const std::vector<uint32_t>& indices) override;
    
    // IRenderBackend shadow interface (stub for SDL_gpu backend)
    void createShadowMap(const std::vector<SceneNode>& nodes) override { (void)nodes; }
    void getShadowMapSize(int& width, int& height) const override { width = 0; height = 0; }
    void fitDirectionalShadowMatrix(Camera& camera, const glm::vec3& lightPosition, int shadowWidth, int shadowHeight, glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix) override { (void)camera; (void)lightPosition; (void)shadowWidth; (void)shadowHeight; (void)lightView; (void)lightProjection; (void)lightSpaceMatrix; }
    void updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos) override { (void)camera; (void)lightSpaceMatrix; (void)lightPos; }
    
    void initUIRendering() override {}
    void shutdownUIRendering() override {}
    void beginUIRender() override {}
    void endUIRender() override {}
    std::unique_ptr<Capture::Frame> captureFrame() override;

private:
    Renderer& owner;
    SDL_Window* window;
    void* device = nullptr;
    void* vbo = nullptr;
    void* ibo = nullptr;
    void* pipeline = nullptr;
    void* ubo = nullptr;
    void* depthTexture = nullptr;
    uint32_t swapchainWidth;
    uint32_t swapchainHeight;
};

#endif // _SDLGPU_BACKEND_H_