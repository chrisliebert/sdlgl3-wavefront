#include "Shader.h"
#include <utility>
#include <algorithm>

namespace {
std::string readTextFileExact(std::string_view path)
{
    std::ifstream fileStream(std::string(path), std::ios::binary);
    if (!fileStream.is_open())
    {
        return {};
    }

    fileStream.seekg(0, std::ios::end);
    const std::streamoff size = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);

    if (size <= 0)
    {
        return {};
    }

    std::string contents(static_cast<std::size_t>(size), '\0');
    fileStream.read(contents.data(), size);
    if (!fileStream)
    {
        return {};
    }
    return contents;
}
}

// ============================================================================
// ShaderCache Implementation (Fix #7.5)
// ============================================================================

std::shared_ptr<Shader> ShaderCache::getShader(std::string_view filePath)
{
    const std::string pathStr(filePath);
    
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // Check if entry exists and is still valid
    auto it = cache.find(pathStr);
    if (it != cache.end())
    {
        try
        {
            const auto currentModified = std::filesystem::last_write_time(pathStr);
            if (currentModified == it->second.lastModified)
            {
                auto shader = it->second.shader;
                if (!shader->isCompiled() && SDL_GL_GetCurrentContext() != nullptr)
                {
                    shader->compile();
                }
                return shader; // Cache hit
            }
        }
        catch (...)
        {
            // File may have been deleted, fall through to recompile
        }
    }
    
    // Cache miss or file changed - compile new shader
    return compileAndCache(pathStr);
}

void ShaderCache::invalidateAll()
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.clear();
}

void ShaderCache::invalidate(std::string_view filePath)
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.erase(std::string(filePath));
}

std::shared_ptr<Shader> ShaderCache::compileAndCache(std::string_view filePath)
{
    const std::string pathStr(filePath);
    
    // Read source file
    std::string sourceCode = readTextFileExact(pathStr);
    if (sourceCode.empty())
    {
        std::cerr << "Unable to load shader: " << pathStr << std::endl;
        return nullptr;
    }
    
    // Determine shader type from file extension
    const std::filesystem::path path(pathStr);
    const std::string ext = path.extension().string();
    
    bool hasContext = SDL_GL_GetCurrentContext() != nullptr;
    
    std::shared_ptr<Shader> shader;
    if (ext == ".frag" || ext == ".fs")
    {
        shader = std::make_shared<FragmentShader>(pathStr, !hasContext);
    }
    else if (ext == ".vert" || ext == ".vs")
    {
        shader = std::make_shared<VertexShader>(pathStr, !hasContext);
    }
    else
    {
        shader = std::make_shared<Shader>(pathStr);
    }
    
    // Store in cache
    CacheEntry entry;
    entry.shader = shader;
    entry.lastModified = std::filesystem::last_write_time(pathStr);
    entry.sourceCode = sourceCode;
    
    cache[pathStr] = entry;
    ++totalCompilations;
    
    return shader;
}

// ============================================================================
// Shader Implementation
// ============================================================================

void FragmentShader::createFragmentShader()
{
    id = glCreateShader(GL_FRAGMENT_SHADER);
    const char* src = shaderSrc.c_str();
    glShaderSource(id, 1, &src, 0);
    glCompileShader(id);
    GLint shaderCompiled;
    glGetShaderiv(id, GL_COMPILE_STATUS, &shaderCompiled);

    if (shaderCompiled == GL_FALSE)
    {
        GLint infologLength = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infologLength);

        std::string log;
        if (infologLength > 1)
        {
            log.resize(static_cast<std::size_t>(infologLength));
            glGetShaderInfoLog(id, infologLength, nullptr, log.data());
            if (!log.empty() && log.back() == '\0') {
                log.pop_back();
            }
        }
        std::cerr << "Fragment shader " << filePath << " failed to compile: " << log << std::endl;
    }
}

void VertexShader::createVertexShader()
{
    id = glCreateShader(GL_VERTEX_SHADER);
    const char* src = shaderSrc.c_str();
    glShaderSource(id, 1, &src, 0);
    glCompileShader(id);
    GLint shaderCompiled;
    glGetShaderiv(id, GL_COMPILE_STATUS, &shaderCompiled);

    if (shaderCompiled == GL_FALSE)
    {
        GLint infologLength = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infologLength);

        std::string log;
        if (infologLength > 1)
        {
            log.resize(static_cast<std::size_t>(infologLength));
            glGetShaderInfoLog(id, infologLength, nullptr, log.data());
            if (!log.empty() && log.back() == '\0') {
                log.pop_back();
            }
        }
        std::cerr << "Vertex shader " << filePath << " failed to compile: " << log << std::endl;
    }
}

void Shader::load(const char* _filePath)
{
    load(std::string_view(_filePath));
}

void Shader::load(std::string_view _filePath)
{
    shaderSrc.clear();
    filePath.assign(_filePath);
    shaderSrc = readTextFileExact(filePath);
    if (shaderSrc.empty())
    {
        std::cerr << "Unable to load shader source: " << filePath << std::endl;
    }
}

Shader::Shader(const char* _filePath)
{
    load(_filePath);
}

Shader::Shader(std::string_view _filePath)
{
    load(_filePath);
}

Shader::Shader()
{
    id = 0;
    filePath.clear();
}

Shader::Shader(Shader&& other) noexcept
{
    id = other.id;
    filePath = std::move(other.filePath);
    shaderSrc = std::move(other.shaderSrc);
    other.id = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        if (id != 0) {
            glDeleteShader(id);
        }
        id = other.id;
        filePath = std::move(other.filePath);
        shaderSrc = std::move(other.shaderSrc);
        other.id = 0;
    }
    return *this;
}

Shader::~Shader()
{
    if (id != 0) {
        glDeleteShader(id);
    }
}

GLuint Shader::getId() const noexcept
{
    return id;
}

std::shared_ptr<Shader> Shader::createFromCache(std::string_view filePath)
{
    return ShaderCache::getInstance().getShader(filePath);
}

FragmentShader::FragmentShader(const char* _filePath)
{
    load(_filePath);
    createFragmentShader();
}

FragmentShader::FragmentShader(std::string_view _filePath, bool deferCompile)
{
    load(_filePath);
    if (!deferCompile)
        createFragmentShader();
}

void FragmentShader::compile()
{
    if (!isCompiled())
        createFragmentShader();
}

FragmentShader::~FragmentShader() = default;

std::shared_ptr<FragmentShader> FragmentShader::createFromCache(std::string_view filePath)
{
    auto shader = ShaderCache::getInstance().getShader(filePath);
    if (auto frag = std::dynamic_pointer_cast<FragmentShader>(shader))
        return frag;
    return nullptr;
}

VertexShader::VertexShader(const char* _filePath)
{
    load(_filePath);
    createVertexShader();
}

VertexShader::VertexShader(std::string_view _filePath, bool deferCompile) : Shader(_filePath)
{
    if (!deferCompile)
        createVertexShader();
}

void VertexShader::compile()
{
    if (!isCompiled())
        createVertexShader();
}

VertexShader::~VertexShader() = default;

std::shared_ptr<VertexShader> VertexShader::createFromCache(std::string_view filePath)
{
    auto shader = ShaderCache::getInstance().getShader(filePath);
    if (auto vert = std::dynamic_pointer_cast<VertexShader>(shader))
        return vert;
    return nullptr;
}
