#include "SdlGpuBackend.h"
#include "Renderer.h"
#include "ConfigLoader.h"
#include "Camera.h"
#include "GpuTypes.h"
#include <iostream>
#include <vector>
#include <fstream>

// Helper to load SPIR-V shader files
static std::vector<char> ReadSpirvFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return {};
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

SdlGpuBackend::SdlGpuBackend(Renderer& renderer) : owner(renderer) {
    window = nullptr;
    device = nullptr;
    vbo = nullptr;
    ibo = nullptr;
    pipeline = nullptr;
    depthTexture = nullptr;
    ubo = nullptr;
    swapchainWidth = 0;
    swapchainHeight = 0;
}

SdlGpuBackend::~SdlGpuBackend() {
    shutdown();
}

static SDL_GpuBuffer* CreateAndUploadBuffer(
    SDL_GpuCommandBuffer* cmdbuf,
    SDL_GpuDevice* device,
    SDL_GpuBufferUsageFlags usage,
    const void* data,
    uint32_t size
) {
    SDL_GpuBufferCreateInfo bufferInfo;
    bufferInfo.usage = usage;
    bufferInfo.sizeInBytes = size;
    bufferInfo.flags = SDL_GPU_BUFFER_FLAG_NONE;

    SDL_GpuBuffer* buffer = SDL_GpuCreateBuffer(device, &bufferInfo);
    if (buffer == nullptr) {
        std::cerr << "Failed to create buffer!" << std::endl;
        return nullptr;
    }

    SDL_GpuTransferBuffer* transferBuffer = SDL_GpuCreateTransferBuffer(
        device,
        SDL_GPU_TRANSFER_BUFFER_USAGE_UPLOAD,
        size
    );

    void* mapping = SDL_GpuMapTransferBuffer(device, transferBuffer, false);
    memcpy(mapping, data, size);
    SDL_GpuUnmapTransferBuffer(device, transferBuffer);

    SDL_GpuCopyPass* copyPass = SDL_GpuBeginCopyPass(cmdbuf);
    SDL_GpuUploadToBuffer(
        copyPass,
        transferBuffer,
        buffer,
        0,
        0,
        size
    );
    SDL_GpuEndCopyPass(copyPass);

    // We can release the transfer buffer after the submit.
    // The GPU will keep it alive until the copy is done.
    SDL_GpuReleaseTransferBuffer(device, transferBuffer);

    return buffer;
}


bool SdlGpuBackend::initialize(SDL_Window* w) {
    window = w;

    // Explicitly request Vulkan
    if (!SDL_GpuSelectBackend(SDL_GPU_BACKEND_VULKAN)) {
        std::cerr << "Failed to select Vulkan backend!" << std::endl;
        return false;
    }

    device = SDL_GpuCreateDevice(window, true);
    if (device == nullptr) {
        std::cerr << "Failed to create SDL_GpuDevice: " << SDL_GetError() << std::endl;
        return false;
    }

    std::cout << "SDL_gpu backend initialized with driver: " << SDL_GpuGetBackendName(SDL_GpuGetBackend()) << std::endl;

    return true;
}

void SdlGpuBackend::shutdown() {
    if (depthTexture != nullptr) {
        SDL_GpuReleaseTexture(device, depthTexture);
        depthTexture = nullptr;
    }
    if (ubo != nullptr) {
        SDL_GpuReleaseBuffer(device, ubo);
        ubo = nullptr;
    }
    if (pipeline != nullptr) {
        SDL_GpuReleaseGraphicsPipeline(device, pipeline);
        pipeline = nullptr;
    }
    if (ibo != nullptr) {
        SDL_GpuReleaseBuffer(device, ibo);
        ibo = nullptr;
    }
    if (vbo != nullptr) {
        SDL_GpuReleaseBuffer(device, vbo);
        vbo = nullptr;
    }
    if (device != nullptr) {
        SDL_GpuDestroyDevice(device);
        device = nullptr;
    }
}

bool SdlGpuBackend::bufferToGpu(Camera& camera)
{
    // Batch all initial uploads into a single command buffer
    SDL_GpuCommandBuffer* cmdbuf = SDL_GpuAcquireCommandBuffer(device);

    // 1. Upload Vertex and Index Buffers
    vbo = CreateAndUploadBuffer(
        cmdbuf,
        device,
        SDL_GPU_BUFFERUSAGE_VERTEX_BIT,
        owner.vertexData.data(),
        (uint32_t)(owner.vertexData.size() * sizeof(Vertex))
    );

    ibo = CreateAndUploadBuffer(
        cmdbuf,
        device,
        SDL_GPU_BUFFERUSAGE_INDEX_BIT,
        owner.indices.data(),
        (uint32_t)(owner.indices.size() * sizeof(GLuint))
    );

    if (vbo == nullptr || ibo == nullptr) {
        SDL_GpuSubmit(cmdbuf); // Submit to release the command buffer
        return false;
    }

    // Create Uniform Buffer
    SDL_GpuBufferCreateInfo uboInfo;
    uboInfo.usage = SDL_GPU_BUFFERUSAGE_UNIFORM_BIT;
    uboInfo.sizeInBytes = sizeof(Uniforms);
    uboInfo.flags = SDL_GPU_BUFFER_FLAG_CPU_TO_GPU; // Frequently updated
    ubo = SDL_GpuCreateBuffer(device, &uboInfo);
    if (ubo == nullptr) {
        SDL_GpuSubmit(cmdbuf);
        return false;
    }

    // 2. Load Shaders
    std::string vertShaderPath = std::string(SHADER_DIRECTORY) + "/" + "shader_vk.vert.spv";
    std::string fragShaderPath = std::string(SHADER_DIRECTORY) + "/" + "shader_vk.frag.spv";

    auto vertSpirv = ReadSpirvFile(vertShaderPath);
    auto fragSpirv = ReadSpirvFile(fragShaderPath);

    if (vertSpirv.empty() || fragSpirv.empty()) {
        std::cerr << "Could not load SPIR-V shaders. Make sure they are compiled." << std::endl;
        return false;
    }

    SDL_GpuShaderCreateInfo vertShaderInfo;
    vertShaderInfo.code = (uint32_t*)vertSpirv.data();
    vertShaderInfo.codeSize = vertSpirv.size();
    vertShaderInfo.entryPointName = "main";
    vertShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;

    SDL_GpuShaderCreateInfo fragShaderInfo;
    fragShaderInfo.code = (uint32_t*)fragSpirv.data();
    fragShaderInfo.codeSize = fragSpirv.size();
    fragShaderInfo.entryPointName = "main";
    fragShaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

    SDL_GpuShader* vertShader = SDL_GpuCreateShader(device, &vertShaderInfo);
    SDL_GpuShader* fragShader = SDL_GpuCreateShader(device, &fragShaderInfo);

    // 3. Create Graphics Pipeline
    SDL_GpuGraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.vertexShader = vertShader;
    pipelineInfo.fragmentShader = fragShader;

    SDL_GpuVertexBinding vertexBinding;
    vertexBinding.binding = 0;
    vertexBinding.stride = sizeof(Vertex);
    vertexBinding.inputRate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    pipelineInfo.vertexInputState.vertexBindingCount = 1;
    pipelineInfo.vertexInputState.vertexBindings = &vertexBinding;

    std::vector<SDL_GpuVertexAttribute> vertexAttributes(3);
    // Position
    vertexAttributes[0].binding = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_VECTOR3;
    vertexAttributes[0].offset = offsetof(Vertex, vertex);
    // Normal
    vertexAttributes[1].binding = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_VECTOR3;
    vertexAttributes[1].offset = offsetof(Vertex, normal);
    // TexCoord
    vertexAttributes[2].binding = 0;
    vertexAttributes[2].location = 2;
    vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_VECTOR2;
    vertexAttributes[2].offset = offsetof(Vertex, textureCoordinate);
    pipelineInfo.vertexInputState.vertexAttributeCount = 3;
    pipelineInfo.vertexInputState.vertexAttributes = vertexAttributes.data();

    pipelineInfo.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizerState.cullMode = owner.configLoader->getBool("cullFace") ? SDL_GPU_CULLMODE_BACK : SDL_GPU_CULLMODE_NONE;
    pipelineInfo.multisampleState.sampleCount = SDL_GPU_SAMPLECOUNT_1; // TODO: From config
    pipelineInfo.depthStencilState.depthTestEnable = true;
    pipelineInfo.depthStencilState.depthWriteEnable = true;
    pipelineInfo.depthStencilState.compareOp = SDL_GPU_COMPAREOP_LESS;

    SDL_GpuSampler* sampler = SDL_GpuCreateSampler(device, nullptr); // Default sampler

    SDL_GpuBinding uboBinding = {};
    uboBinding.binding = 0;
    uboBinding.resource.type = SDL_GPU_RESOURCETYPE_UNIFORM_BUFFER;
    uboBinding.resource.uniformBuffer.buffer = ubo;

    SDL_GpuBinding textureBinding = {};
    textureBinding.binding = 1;
    textureBinding.resource.type = SDL_GPU_RESOURCETYPE_SAMPLER_AND_TEXTURE;
    textureBinding.resource.samplerAndTexture.sampler = sampler;
    // texture will be bound per-draw

    SDL_GpuTextureFormat colorFormat = SDL_GpuGetSwapchainTextureFormat(device, window);
    pipelineInfo.colorAttachmentCount = 1;
    pipelineInfo.colorAttachmentFormats = &colorFormat;
    pipelineInfo.depthStencilAttachmentFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;

    pipeline = SDL_GpuCreateGraphicsPipeline(device, &pipelineInfo);

    // Clean up temporary objects
    SDL_GpuSubmit(cmdbuf); // Submit the batched uploads
    SDL_GpuReleaseShader(device, vertShader);
    SDL_GpuReleaseShader(device, fragShader);
    SDL_GpuReleaseSampler(device, sampler);

    return pipeline != nullptr;
}

void SdlGpuBackend::beginFrame(const FrameContext& frameContext) {
    // This is now part of the submit logic, so this can be a no-op for now.
}

void SdlGpuBackend::submit(const std::vector<RenderCommand>& commands) {
    SDL_GpuCommandBuffer* cmdbuf = SDL_GpuAcquireCommandBuffer(device);
    if (cmdbuf == nullptr) return;

    SDL_GpuTexture* swapchainTexture = SDL_GpuAcquireSwapchainTexture(cmdbuf, window);
    if (swapchainTexture == nullptr) {
        // This can happen if the window is minimized.
        SDL_GpuSubmit(cmdbuf);
        return;
    }

    // Recreate depth buffer if swapchain has been resized
    if (depthTexture == nullptr || swapchainWidth != swapchainTexture->width || swapchainHeight != swapchainTexture->height)
    {
        if (depthTexture != nullptr) {
            SDL_GpuReleaseTexture(device, depthTexture);
        }
        SDL_GpuTextureCreateInfo depthTextureInfo;
        depthTextureInfo.width = swapchainTexture->width;
        depthTextureInfo.height = swapchainTexture->height;
        depthTextureInfo.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
        depthTextureInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET_BIT;
        depthTexture = SDL_GpuCreateTexture(device, &depthTextureInfo);
        swapchainWidth = swapchainTexture->width;
        swapchainHeight = swapchainTexture->height;
    }

    SDL_GpuColorAttachmentInfo colorAttachment;
    colorAttachment.texture = swapchainTexture;
    colorAttachment.loadOp = SDL_GPU_LOADOP_CLEAR;
    colorAttachment.storeOp = SDL_GPU_STOREOP_STORE;
    colorAttachment.clearColor.r = 0.5f;
    colorAttachment.clearColor.g = 0.2f;
    colorAttachment.clearColor.b = 0.8f;
    colorAttachment.clearColor.a = 1.0f;

    SDL_GpuDepthStencilAttachmentInfo depthAttachment;
    depthAttachment.texture = depthTexture;
    depthAttachment.loadOp = SDL_GPU_LOADOP_CLEAR;
    depthAttachment.storeOp = SDL_GPU_STOREOP_DONT_CARE;
    depthAttachment.depthClearValue = 1.0f;

    SDL_GpuRenderPass* pass = SDL_GpuBeginRenderPass(cmdbuf, &colorAttachment, 1, &depthAttachment);

    SDL_GpuBindGraphicsPipeline(pass, pipeline);

    SDL_GpuBindVertexBuffer(pass, 0, vbo, 0);
    SDL_GpuBindIndexBuffer(pass, ibo, 0, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    // Update and bind the UBO once for the whole frame
    Uniforms uniforms;
    const auto& perfStats = owner.getPerfStats();
    uniforms.projection = perfStats.camera->projectionMatrix;
    uniforms.view = perfStats.camera->modelViewMatrix;
    SDL_GpuUploadToBuffer(pass, ubo, 0, &uniforms, sizeof(Uniforms));

    SDL_GpuBindUniformBuffer(pass, 0, ubo, 0, sizeof(Uniforms));

    for (const auto& command : commands) {
        const SceneNode* node = command.node;

        SDL_GpuDrawIndexed(
            pass,
            node->startPosition,
            node->endPosition - node->startPosition,
            1,
            0,
            0
        );
    }

    SDL_GpuEndRenderPass(pass);

    SDL_GpuSubmit(cmdbuf);

    // NOTE: We no longer release the depth texture here. It's persistent.
}

void SdlGpuBackend::endFrame() {
    // Presentation is handled by SDL_GpuSubmit in submit()
}

void SdlGpuBackend::takeScreenshot(const char* filename)
{
    if (device == nullptr) return;

    SDL_Surface* surface = SDL_GpuReadBackSwapchain(device, window);
    if (surface == nullptr)
    {
        std::cerr << "Failed to read back swapchain for screenshot: " << SDL_GetError() << std::endl;
        return;
    }

    if (SDL_SaveBMP(surface, filename) != 0)
    {
        std::cerr << "Failed to save screenshot: " << SDL_GetError() << std::endl;
    }
    else
    {
        std::cout << "Screenshot saved to " << filename << std::endl;
    }

    SDL_DestroySurface(surface);
}