#ifndef _SHADER_H_
#define _SHADER_H_

#include "Common.h"
#include <string_view>
#include <memory>
#include <filesystem>
#include <mutex>
#include <unordered_map>

class Shader;  // Forward declaration for circular dependency

/**
 * @class ShaderCache
 * @brief Thread-safe shader compilation cache with file modification tracking
 * 
 * Prevents redundant shader recompilation by caching compiled shaders
 * and checking file modification times. Uses a singleton pattern for
 * global access across the application.
 */
class ShaderCache {
public:
    // Singleton access
    static ShaderCache& getInstance() {
        static ShaderCache instance;
        return instance;
    }
    
    // Non-copyable, non-movable
    ShaderCache(const ShaderCache&) = delete;
    ShaderCache& operator=(const ShaderCache&) = delete;
    ShaderCache(ShaderCache&&) = delete;
    ShaderCache& operator=(ShaderCache&&) = delete;
    
    /**
     * @brief Get or compile a shader from file
     * @param filePath Path to the shader source file
     * @return Shared pointer to the compiled shader
     */
    std::shared_ptr<Shader> getShader(std::string_view filePath);
    
    /**
     * @brief Force recompilation of all cached shaders
     */
    void invalidateAll();
    
    /**
     * @brief Invalidate a specific shader cache entry
     * @param filePath Path to the shader to invalidate
     */
    void invalidate(std::string_view filePath);
    
    /**
     * @brief Get cache statistics
     * @return Pair of (cached count, total compilations)
     */
    std::pair<size_t, size_t> getStats() const {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return std::make_pair(cache.size(), totalCompilations);
    }

private:
    ShaderCache() = default;
    ~ShaderCache() = default;
    
    struct CacheEntry {
        std::shared_ptr<Shader> shader;
        std::filesystem::file_time_type lastModified;
        std::string sourceCode;
    };
    
    mutable std::mutex cacheMutex;
    std::unordered_map<std::string, CacheEntry> cache;
    size_t totalCompilations = 0;
    
    std::shared_ptr<Shader> compileAndCache(std::string_view filePath);
};

class Shader
{
public:
    Shader(const char* _filePath);
    Shader(std::string_view _filePath);
    Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;
    virtual ~Shader();
    
    void load(const char* _filePath);
    void load(std::string_view _filePath);
    [[nodiscard]] GLuint getId() const noexcept;
    
    // Use shader caching for production code
    static std::shared_ptr<Shader> createFromCache(std::string_view filePath);

protected:
    GLuint id = 0;
    std::string filePath;
    std::string shaderSrc;
};

class FragmentShader : public Shader
{
public:
    FragmentShader(const char* _filePath);
    FragmentShader(std::string_view _filePath);
    ~FragmentShader();
    
    static std::shared_ptr<FragmentShader> createFromCache(std::string_view filePath);
    
protected:
    void createFragmentShader();
};

class VertexShader : public Shader
{
public:
    VertexShader(const char* _filePath);
    VertexShader(std::string_view _filePath);
    ~VertexShader();
    
    static std::shared_ptr<VertexShader> createFromCache(std::string_view filePath);
    
protected:
    void createVertexShader();
};

#endif // _SHADER_H_
