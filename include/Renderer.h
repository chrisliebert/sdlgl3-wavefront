#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "Camera.h"
#include "Common.h"
#include "Frustum.h"
#include "RenderBackend.h"
#include "GpuProgram.h"
#include "Material.h"
#include "SceneNode.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

#if __has_include(<SDL3_image/SDL_image.h>)
#include <SDL3_image/SDL_image.h>
#elif __has_include(<SDL_image.h>)
#include <SDL_image.h>
#else
#error "SDL_image headers not found"
#endif

#if __has_include(<SDL3/SDL_thread.h>)
#include <SDL3/SDL_thread.h>
#elif __has_include(<SDL_thread.h>)
#include <SDL_thread.h>
#endif

// GLAD_DEBUG logging (only active when GLAD_DEBUG is defined)
#ifdef GLAD_DEBUG
void pre_gl_call(const char* name, void* funcptr, int len_args, ...);
#endif

class ConfigLoader;

/**
 * @class Renderer
 * @brief High-level scene rendering manager with frustum culling and shadow mapping
 * 
 * Manages scene graph data, GPU buffer allocation, and rendering pipeline.
 * Thread-safe for scene loading via mutex protection.
 */
class Renderer
{
public:
    struct PerfStats {
        double cpuFrameMs = 0.0;
        double shadowPassMs = 0.0;
        double fps = 0.0;
        int drawCalls = 0;
        int visibleNodes = 0;
        int totalNodes = 0;
        int frustumCulledNodes = 0;
        int occlusionCulledNodes = 0;
        const Camera* camera = nullptr;
    };

    Renderer();
    ~Renderer();
    
    // Non-copyable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    void addMaterial(std::string_view name, const Material& material);
    void addSceneNode(SceneNode node);
    void addTexture(std::string_view textureFileName, GLuint* textureId, std::unique_ptr<SDL_Surface, SdlSurfaceDeleter> surface);
    void addTexture(std::string_view textureFileName, GLuint* textureId, const Texture* texture);
    void addTexture(std::string_view textureFileName, GLuint* textureId);
    void addWavefront(std::string_view fileName, const glm::mat4& matrix);
    void setBackend(std::unique_ptr<IRenderBackend> backend);
    
    [[nodiscard]] bool buildScene(Camera& camera);
    [[nodiscard]] bool buildScene(Camera& camera, std::string_view cacheFilename);

    void bufferToGpu(Camera& camera, bool loadCachedScene);
    [[nodiscard]] bool checkScene() const;
    void render(Camera& camera, const FrameContext& frameContext);
    void enableShadows();
    void disableShadows();
    GLuint createShadowMap(Camera& camera);
    
    [[nodiscard]] IRenderBackend* getBackend() const { return backend.get(); }
    [[nodiscard]] const PerfStats& getPerfStats() const { return perfStats; }
    [[nodiscard]] bool isProfilerEnabled() const { return profilerEnabled; }
    [[nodiscard]] bool isVerboseEnabled() const;
    [[nodiscard]] bool getShadowsEnabled() const { return shadowsEnabled; }

    // Public data access (for backend use) - protected by sceneDataMutex during writes
    std::vector<SceneNode> sceneNodes;
    std::vector<Vertex> vertexData;
    std::vector<GLuint> indices;
    std::unordered_map<std::string, Material> materials;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
    std::unique_ptr<ConfigLoader> configLoader;
    std::string cacheFileName;

private:
    struct CullNode {
        float cx = 0.0f, cy = 0.0f, cz = 0.0f;
        float radius = 0.0f;
        std::size_t leftChild = static_cast<std::size_t>(-1);
        std::size_t rightChild = static_cast<std::size_t>(-1);
        std::size_t firstLeaf = static_cast<std::size_t>(-1);
        int leafCount = 0;
    };

    // Thread-safe scene loading
    mutable std::mutex sceneDataMutex;
    
    int buildCullNode(std::vector<int>& sortedIndices, int start, int end);
    void resolveTextures();
    void rebuildScenegraph();
    void collectVisibleNodes(std::vector<int>& outVisible);
    void updateOcclusionQueryResults();
    std::shared_ptr<Texture> textureFromSurface(std::unique_ptr<SDL_Surface, SdlSurfaceDeleter> image);
    int createBinCacheInternal();
    static int createBinCacheThread(void* rendererPtr) {
        return static_cast<Renderer*>(rendererPtr)->createBinCacheInternal();
    }


    bool shadowsEnabled = false;
    bool profilerEnabled = false;
    GLuint shadowMap = 0;
    int shadowWidth = 2048;
    int shadowHeight = 2048;
    bool hierarchicalCullingEnabled = true;
    bool occlusionCullingEnabled = true;
    int cullLeafSize = 16;
    int occlusionRetestFrames = 8;
    int occlusionMinSamples = 1;
    bool frustumCullingEnabled = true;
    
    PerfStats perfStats{};
    Uint64 lastPerfCounter = 0;
    Frustum frustum;
    void* binCacheWriterThread = nullptr;
    std::vector<CullNode> cullNodes;
    std::vector<int> cullLeafNodeIndices;
    std::vector<GLuint> occlusionQueries;
    std::vector<unsigned char> occlusionVisible;
    std::vector<int> occlusionSkipCounters;
    std::vector<int> visibleNodeIdsScratch;
    struct SortKey { GLuint textureId; GLuint startPosition; int nodeIndex; };
    std::vector<SortKey> visibleNodesSortedScratch;
    std::vector<SortKey> occlusionFilteredScratch;
    std::vector<RenderCommand> renderCommandsScratch;

    bool shadowDirty = true;
    bool shadowInitialized = false;
    glm::vec3 lastShadowLightPos{0.0f, 0.0f, 0.0f};
    size_t sceneRevision = 0;
    size_t shadowSceneRevision = 0;
    std::unique_ptr<IRenderBackend> backend;
};

#endif // _RENDERER_H_
