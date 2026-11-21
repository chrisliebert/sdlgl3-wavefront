#pragma once

#include "RenderBackend.h"
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>

#ifdef HAVE_VULKAN
#include <vulkan/vulkan.h>
#include "VulkanHelpers.h"
#endif

class Renderer;
struct Texture;
struct SDL_Surface;

struct VulkanFrameUniforms
{
    glm::mat4 projection{1.0f};
    glm::mat4 view{1.0f};
    glm::mat4 lightSpaceMatrix{1.0f};
    glm::vec4 lightPos{10.0f, 50.0f, 0.0f, 1.0f};
};

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
    glm::mat4 m_lightSpaceMatrix{1.0f};
    glm::vec3 m_lightPos{10.0f, 50.0f, 0.0f};

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
    VkFormat findDepthFormat();
    void createDepthResources();
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

    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat m_depthFormat = VK_FORMAT_UNDEFINED;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<vkhelpers::VulkanTexture> m_textures;
    VkSampler m_sharedSampler = VK_NULL_HANDLE;

    VkBuffer m_frameUniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_frameUniformBufferMemory = VK_NULL_HANDLE;
    VulkanFrameUniforms m_frameUniforms{};

    VkRenderPass m_shadowRenderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_shadowPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_shadowPipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_shadowDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_shadowDescriptorSet = VK_NULL_HANDLE;
    VkImage m_shadowDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_shadowDepthImageMemory = VK_NULL_HANDLE;
    VkImageView m_shadowDepthImageView = VK_NULL_HANDLE;
    VkSampler m_shadowSampler = VK_NULL_HANDLE;
    VkFramebuffer m_shadowFramebuffer = VK_NULL_HANDLE;
    VkFormat m_shadowDepthFormat = VK_FORMAT_UNDEFINED;
    uint32_t m_shadowWidth = 2048;
    uint32_t m_shadowHeight = 2048;
    bool m_shadowResourcesReady = false;

    struct TextureCacheKey {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 1;
        uint64_t pixelHash = 0;

        bool operator==(const TextureCacheKey& other) const {
            return width == other.width &&
                   height == other.height &&
                   mipLevels == other.mipLevels &&
                   pixelHash == other.pixelHash;
        }
    };

    struct TextureCacheKeyHasher {
        size_t operator()(const TextureCacheKey& key) const noexcept {
            const size_t h1 = std::hash<uint32_t>{}(key.width);
            const size_t h2 = std::hash<uint32_t>{}(key.height);
            const size_t h3 = std::hash<uint32_t>{}(key.mipLevels);
            const size_t h4 = std::hash<uint64_t>{}(key.pixelHash);
            return (((h1 * 1315423911u) ^ (h2 << 1)) ^ (h3 << 2)) ^ (h4 << 3);
        }
    };

    std::unordered_map<TextureCacheKey, GLuint, TextureCacheKeyHasher> m_textureCache;

    uint32_t m_imageIndex = 0;
    bool m_frameReady = false;

    VkSampler getOrCreateSharedSampler(uint32_t mipLevels);
    void updateAllDescriptorSets();
    void createFrameUniformBuffer();
    void updateFrameUniformBuffer();

    void createShadowResources();
    void destroyShadowResources();
    void createShadowRenderPass();
    void createShadowPipeline();
    void createShadowDepthImage();
    void createShadowFramebuffer();
    void renderShadowPass(const std::vector<SceneNode>& nodes);

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
