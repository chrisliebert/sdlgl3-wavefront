#include "VulkanBackend.h"
#include "Renderer.h"
#include "Capture.h"
#include "SceneNode.h"
#include "Camera.h"
#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <vector>

#ifdef HAVE_VULKAN
#include <SDL3/SDL_vulkan.h>

struct PushConstants {
    glm::mat4 model;
    glm::mat4 projection;
};

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file " + filename);
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shaderModule;
}

#endif

VulkanBackend::VulkanBackend(Renderer& renderer)
    : m_renderer(renderer)
{
}

VulkanBackend::~VulkanBackend()
{
    shutdown();
}

bool VulkanBackend::initialize(SDL_Window* window)
{
    m_window = window;

#ifdef HAVE_VULKAN
    try {
        createInstance();
        if (!SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface)) {
            throw std::runtime_error("failed to create window surface!");
        }
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createDescriptorPool();
        createCommandBuffer();
        
    createSyncObjects();

    // Create a default 1x1 white texture
    SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
    if (surface) {
        uint32_t* pixels = (uint32_t*)surface->pixels;
        *pixels = 0xFFFFFFFF; // White
        uint32_t defaultTexId = 0;
        addTexture(&defaultTexId, surface);
        SDL_DestroySurface(surface);
    }

    } catch (const std::exception& e) {
        std::cerr << "Vulkan Initialization Error: " << e.what() << std::endl;
        return false;
    }
    std::cout << "Vulkan Backend initialized." << std::endl;
#else
    std::cerr << "Vulkan support not compiled in!" << std::endl;
#endif

    return true;
}

void VulkanBackend::shutdown()
{
#ifdef HAVE_VULKAN
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
        
        for (auto& tex : m_textures) {
            if(tex.view) vkDestroyImageView(m_device, tex.view, nullptr);
            if(tex.image) vkDestroyImage(m_device, tex.image, nullptr);
            if(tex.memory) vkFreeMemory(m_device, tex.memory, nullptr);
        }
        m_textures.clear();
        
        if (m_vertexBuffer) vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        if (m_vertexBufferMemory) vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        if (m_indexBuffer) vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        if (m_indexBufferMemory) vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

        vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
        vkDestroyFence(m_device, m_inFlightFence, nullptr);

        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        
        for (auto framebuffer : m_framebuffers) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        for (auto imageView : m_swapchainImageViews) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        
        vkDestroyDevice(m_device, nullptr);
        SDL_Vulkan_DestroySurface(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);
        m_device = VK_NULL_HANDLE;
    }
#endif
}

void VulkanBackend::submit(const std::vector<RenderCommand>& commands)
{
#ifdef HAVE_VULKAN
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    VkBuffer vertexBuffers[] = {m_vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(m_commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    for (const auto& cmd : commands) {
        if (!cmd.node) continue;
        PushConstants pc{};
        pc.model = m_viewMatrix * cmd.node->modelViewMatrix;
        pc.projection = m_projectionMatrix; pc.projection[1][1] *= -1.0f;
        vkCmdPushConstants(m_commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pc);
        
        
        VkDescriptorSet ds = VK_NULL_HANDLE;
        if (cmd.node->diffuseTextureId > 0 && cmd.node->diffuseTextureId <= m_textures.size()) {
            ds = m_textures[cmd.node->diffuseTextureId - 1].descriptorSet;
        } else if (!m_textures.empty()) {
            ds = m_textures[0].descriptorSet;
        }
        
        if (ds != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &ds, 0, nullptr);
        }

        
        uint32_t indexCount = cmd.node->endPosition - cmd.node->startPosition;
        if (indexCount > 0) {
            vkCmdDrawIndexed(m_commandBuffer, indexCount, 1, cmd.node->startPosition, 0, 0);
        }
    }
#endif
}

bool VulkanBackend::bufferToGpu(const std::vector<Vertex>& vertexData, const std::vector<uint32_t>& indices)
{
#ifdef HAVE_VULKAN
    if (vertexData.empty() || indices.empty()) return true;

    VkDeviceSize bufferSize = sizeof(vertexData[0]) * vertexData.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vkhelpers::createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertexData.data(), (size_t) bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    vkhelpers::createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);
    vkhelpers::copyBuffer(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_vertexBuffer, bufferSize);
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    bufferSize = sizeof(indices[0]) * indices.size();
    vkhelpers::createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(m_device, stagingBufferMemory);
    vkhelpers::createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);
    vkhelpers::copyBuffer(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, m_indexBuffer, bufferSize);
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
#endif
    return true;
}

void VulkanBackend::createShadowMap(const std::vector<SceneNode>& nodes) {}
void VulkanBackend::getShadowMapSize(int& width, int& height) const { width = 1024; height = 1024; }
void VulkanBackend::fitDirectionalShadowMatrix(Camera& camera, const glm::vec3& lightPosition, int shadowWidth, int shadowHeight, glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix) {}
void VulkanBackend::updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos) {
    m_projectionMatrix = camera.projectionMatrix;
    m_viewMatrix = camera.modelViewMatrix;
}

void VulkanBackend::beginFrame(const struct FrameContext& frameContext)
{
#ifdef HAVE_VULKAN
    vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFence);
    vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &m_imageIndex);
    vkResetCommandBuffer(m_commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_framebuffers[m_imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
#endif
}

void VulkanBackend::endFrame()
{
#ifdef HAVE_VULKAN
    vkCmdEndRenderPass(m_commandBuffer);
    vkEndCommandBuffer(m_commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapchains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &m_imageIndex;
    vkQueuePresentKHR(m_presentQueue, &presentInfo);
#endif
}

void VulkanBackend::initUIRendering() {}
void VulkanBackend::shutdownUIRendering() {}
void VulkanBackend::beginUIRender() {}
void VulkanBackend::endUIRender() {}
std::unique_ptr<Capture::Frame> VulkanBackend::captureFrame() { return nullptr; }

void VulkanBackend::addTexture(GLuint* textureId, SDL_Surface* surface) {
#ifdef HAVE_VULKAN
    if (!surface) return;
    Texture tex;
    tex.width = surface->w;
    tex.height = surface->h;
    tex.bpp = 4;
    tex.data = std::shared_ptr<unsigned char[]>(new unsigned char[surface->w * surface->h * 4]);
    memcpy(tex.data.get(), surface->pixels, surface->w * surface->h * 4);
    tex.mode = GL_RGBA;
    addTexture(textureId, &tex);
#endif
}
void VulkanBackend::addTexture(GLuint* textureId, const Texture* texture) {
#ifdef HAVE_VULKAN
    if (!texture || !texture->isValid()) return;
    
    VkDeviceSize imageSize = texture->width * texture->height * 4; // Vulkan path uploads RGBA8
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vkhelpers::createBuffer(m_physicalDevice, m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);

    const uint8_t* src = texture->data.get();
    uint8_t* dst = static_cast<uint8_t*>(data);
    const size_t pixelCount = static_cast<size_t>(texture->width) * static_cast<size_t>(texture->height);

    if (texture->bpp == 4) {
        std::memcpy(dst, src, static_cast<size_t>(imageSize));
    } else if (texture->bpp == 3 || texture->mode == 3) {
        for (size_t i = 0; i < pixelCount; ++i) {
            dst[i * 4 + 0] = src[i * 3 + 0];
            dst[i * 4 + 1] = src[i * 3 + 1];
            dst[i * 4 + 2] = src[i * 3 + 2];
            dst[i * 4 + 3] = 255;
        }
    } else if (texture->bpp == 2) {
        // Treat 2-channel textures as luminance+alpha.
        for (size_t i = 0; i < pixelCount; ++i) {
            const uint8_t l = src[i * 2 + 0];
            dst[i * 4 + 0] = l;
            dst[i * 4 + 1] = l;
            dst[i * 4 + 2] = l;
            dst[i * 4 + 3] = src[i * 2 + 1];
        }
    } else if (texture->bpp == 1) {
        // Treat single-channel textures as grayscale.
        for (size_t i = 0; i < pixelCount; ++i) {
            const uint8_t l = src[i];
            dst[i * 4 + 0] = l;
            dst[i * 4 + 1] = l;
            dst[i * 4 + 2] = l;
            dst[i * 4 + 3] = 255;
        }
    } else {
        std::cerr << "Unsupported texture bpp in Vulkan upload: " << texture->bpp << std::endl;
        std::memset(dst, 255, static_cast<size_t>(imageSize));
    }
    
    vkUnmapMemory(m_device, stagingBufferMemory);
    
    VkImage image;
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = texture->width;
    imageInfo.extent.height = texture->height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(m_device, &imageInfo, nullptr, &image);

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vkhelpers::findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkDeviceMemory imageMemory;
    vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory);
    vkBindImageMemory(m_device, image, imageMemory, 0);

    vkhelpers::transitionImageLayout(m_device, m_commandPool, m_graphicsQueue, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkhelpers::copyBufferToImage(m_device, m_commandPool, m_graphicsQueue, stagingBuffer, image, texture->width, texture->height);
    vkhelpers::transitionImageLayout(m_device, m_commandPool, m_graphicsQueue, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    VkImageView view;
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(m_device, &viewInfo, nullptr, &view);

    VkSampler sampler;
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler);

    VkDescriptorSet descriptorSet;
    VkDescriptorSetAllocateInfo dallocInfo{};
    dallocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dallocInfo.descriptorPool = m_descriptorPool;
    dallocInfo.descriptorSetCount = 1;
    dallocInfo.pSetLayouts = &m_descriptorSetLayout;
    vkAllocateDescriptorSets(m_device, &dallocInfo, &descriptorSet);

    VkDescriptorImageInfo descImageInfo{};
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descImageInfo.imageView = view;
    descImageInfo.sampler = sampler;
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &descImageInfo;
    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

    vkhelpers::VulkanTexture vtex;
    vtex.image = image;
    vtex.memory = imageMemory;
    vtex.view = view;
    vtex.descriptorSet = descriptorSet;
    m_textures.push_back(vtex);

    *textureId = static_cast<GLuint>(m_textures.size());
#endif
}

#ifdef HAVE_VULKAN
void VulkanBackend::createInstance() {
    uint32_t extensionCount = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "sdlgl3-wavefront";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void VulkanBackend::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) throw std::runtime_error("failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    m_physicalDevice = devices[0];
}

void VulkanBackend::createLogicalDevice() {
    m_graphicsQueueFamily = 0; // Assume 0 for now
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
    m_presentQueue = m_graphicsQueue;
}

void VulkanBackend::createSwapchain() {
    int width, height;
    SDL_GetWindowSizeInPixels(m_window, &width, &height);
    m_swapchainExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    m_swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB; // Assume
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = 2;
    createInfo.imageFormat = m_swapchainFormat;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = m_swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());
}

void VulkanBackend::createImageViews() {
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]);
    }
}

void VulkanBackend::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
}

void VulkanBackend::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);
}

void VulkanBackend::createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vulkan.vert.spv");
    auto fragShaderCode = readFile("shaders/vulkan.frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(m_device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(m_device, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, vertex);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, textureCoordinate);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_swapchainExtent.width;
    viewport.height = (float) m_swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void VulkanBackend::createFramebuffers() {
    m_framebuffers.resize(m_swapchainImageViews.size());
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageView attachments[] = { m_swapchainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;
        vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffers[i]);
    }
}

void VulkanBackend::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
    vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
}

void VulkanBackend::createCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer);
}

void VulkanBackend::createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore);
    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);
    vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence);
}

void VulkanBackend::createDescriptorPool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1000;
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1000;
    vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);
}
#endif
