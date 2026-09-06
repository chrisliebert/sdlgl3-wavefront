#pragma once

#include "RenderBackend.h"
#include <glm/glm.hpp>

#ifdef HAVE_VULKAN
#include <vulkan/vulkan.h>
#include "VulkanHelpers.h"
#endif

class Renderer;
struct Texture;
struct SDL_Surface;

class VulkanBackend : public IRenderBackend
{
public:
    VulkanBackend(Renderer& renderer);
    ~VulkanBackend() override;

    bool initialize(SDL_Window* window) override;
    void shutdown() override;

    void submit(const std::vector<RenderCommand>& commands) override;

    bool bufferToGpu(const std::vector<Vertex>& vertexData, const std::vector<uint32_t>& indices) override;
    void createShadowMap(const std::vector<SceneNode>& nodes) override;
    void getShadowMapSize(int& width, int& height) const override;
    void fitDirectionalShadowMatrix(Camera& camera, const glm::vec3& lightPosition, int shadowWidth, int shadowHeight, glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix) override;
    void updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos) override;
    void beginFrame(const struct FrameContext& frameContext) override;
    void endFrame() override;

    void initUIRendering() override;
    void shutdownUIRendering() override;
    void beginUIRender() override;
    void endUIRender() override;

    std::unique_ptr<Capture::Frame> captureFrame() override;

    void addTexture(GLuint* textureId, SDL_Surface* surface);
    void addTexture(GLuint* textureId, const Texture* texture);

private:
    Renderer& m_renderer;
    SDL_Window* m_window = nullptr;

    glm::mat4 m_projectionMatrix{1.0f};
    glm::mat4 m_viewMatrix{1.0f};

#ifdef HAVE_VULKAN
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = 0;
    
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapchainFormat;
    VkExtent2D m_swapchainExtent;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_framebuffers;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence m_inFlightFence = VK_NULL_HANDLE;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<vkhelpers::VulkanTexture> m_textures;
    
    uint32_t m_imageIndex = 0;
    
    void createInstance();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();
    void createSyncObjects();
    void createDescriptorPool();
#endif
};
