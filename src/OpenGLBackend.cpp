#include "OpenGLBackend.h"
#include "Renderer.h"
#include "ConfigLoader.h"
#include "Camera.h"
#include "SceneNode.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

// Forward declare the GL error checking function from Renderer.cpp
inline void _checkForGLSLError(GLuint programId) {
    GLint success;
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(programId, 512, NULL, infoLog);
        std::cerr << "GLSL Link Error: " << infoLog << std::endl;
    }
}

// ============================================================================
// OpenGLBackend Implementation
// ============================================================================

OpenGLBackend::OpenGLBackend(Renderer& renderer)
    : owner(renderer)
{
}

OpenGLBackend::~OpenGLBackend()
{
    shutdown();
}

bool OpenGLBackend::initialize(SDL_Window* window)
{
    m_window = window;
    
    auto& config = owner.configLoader;

    // --- Start of moved code from main.cpp ---
    struct GlContextVersion { int major; int minor; };
    constexpr std::array<GlContextVersion, 4> candidates = {{
        {4, 6}, {4, 5}, {4, 3}, {3, 3}
    }};

    m_glContext = nullptr;
    for (const auto& candidate : candidates)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, candidate.major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, candidate.minor);
        m_glContext = SDL_GL_CreateContext(m_window);
        if (m_glContext != nullptr)
        {
            std::cout << "Requested and received OpenGL " << candidate.major << "." << candidate.minor << " core context" << std::endl;
            break;
        }
    }

    if (m_glContext == nullptr)
    {
        std::cerr << "Failed to create any suitable OpenGL context." << std::endl;
        return false;
    }

    SDL_GL_SetSwapInterval(0);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
    {
        std::cerr << "Something went wrong initializing GLAD!" << std::endl;
        return false;
    }

    if (GLVersion.major < 3 || (GLVersion.major == 3 && GLVersion.minor < 3))
    {
        std::cerr << "Your system doesn't support OpenGL >= 3.3 core features!" << std::endl;
        return false;
    }
    // --- End of moved code ---

    usePersistentMappedVbo = config->hasVar("renderer.vbo.persistent") ? config->getBool("renderer.vbo.persistent") : false;
    verboseLogging = config->hasVar("renderer.verbose") ? config->getBool("renderer.verbose") : false;
    cullFaceEnabled = config->hasVar("cullFace") ? config->getBool("cullFace") : false;
    clearR = config->hasVar("renderer.clearColor.r") ? config->getFloat("renderer.clearColor.r") : 1.0f;
    clearG = config->hasVar("renderer.clearColor.g") ? config->getFloat("renderer.clearColor.g") : 0.8f;
    clearB = config->hasVar("renderer.clearColor.b") ? config->getFloat("renderer.clearColor.b") : 0.8f;
    clearA = config->hasVar("renderer.clearColor.a") ? config->getFloat("renderer.clearColor.a") : 1.0f;
    depthVertShaderPath = (std::filesystem::path(SHADER_DIRECTORY) / std::string(config->getVar("shader.depth.vert"))).string();
    depthFragShaderPath = (std::filesystem::path(SHADER_DIRECTORY) / std::string(config->getVar("shader.depth.frag"))).string();
    mainVertShaderPath = (std::filesystem::path(SHADER_DIRECTORY) / std::string(config->getVar("shader.vert"))).string();
    mainFragShaderPath = (std::filesystem::path(SHADER_DIRECTORY) / std::string(config->getVar("shader.frag"))).string();
    
    shadowWidthW = config->getInt("shadow.width");
    shadowHeightH = config->getInt("shadow.height");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    if (cullFaceEnabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    glClearColor(clearR, clearG, clearB, clearA);
    return true;
}

void OpenGLBackend::initUIRendering()
{
    // Stub
}

void OpenGLBackend::shutdownUIRendering()
{
    // Stub
}

void OpenGLBackend::beginUIRender()
{
    // Stub
}

void OpenGLBackend::endUIRender()
{
    // Stub
}

std::unique_ptr<Capture::Frame> OpenGLBackend::captureFrame()
{
    // Stub
    return nullptr;
}

void OpenGLBackend::shutdown()
{
    // Unmap persistent VBO if active
    if (persistentVboActive && persistentVboPtr)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        persistentVboPtr = nullptr;
        persistentVboActive = false;
    }

    // Delete OpenGL resources
    if (vbo != 0) glDeleteBuffers(1, &vbo);
    if (ibo != 0) glDeleteBuffers(1, &ibo);
    if (vao != 0) glDeleteVertexArrays(1, &vao);
    
    if (shadowMap != 0) glDeleteTextures(1, &shadowMap);
    if (fallbackWhiteTexture != 0) glDeleteTextures(1, &fallbackWhiteTexture);
    if (fallbackNormalTexture != 0) glDeleteTextures(1, &fallbackNormalTexture);
    if (depthMapFBO != 0) glDeleteFramebuffers(1, &depthMapFBO);

    if (gpuProgram) { gpuProgram.reset(); }
    if (shadowProgram) { shadowProgram.reset(); }

    if (m_glContext != nullptr)
    {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }
}

void OpenGLBackend::submit(const std::vector<RenderCommand>& commands)
{
    lastSubmittedCommands = commands;
    if (commands.empty()) return;

    GLint prevVao = 0;
    GLint prevArrayBuffer = 0;
    GLint prevElementArrayBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementArrayBuffer);

    checkForGLError(); glBindVertexArray(vao); checkForGLError();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    
    if (!gpuProgram) {
        fprintf(stderr, "GPU program not initialized\n");
        return;
    }
    
    gpuProgram->use(); checkForGLError();
    gpuProgram->getUniformLoader()->load(); checkForGLError();

    GLuint currentDiffuseTextureId = static_cast<GLuint>(-1);
    GLuint currentNormalTextureId = static_cast<GLuint>(-1);
    GLuint currentSpecularTextureId = static_cast<GLuint>(-1);

    auto* uniformLoader = gpuProgram->getUniformLoader();
    auto* hasNormalUniform = uniformLoader ? dynamic_cast<UniformInt*>(uniformLoader->get("hasNormalTexture")) : nullptr;
    auto* hasSpecularUniform = uniformLoader ? dynamic_cast<UniformInt*>(uniformLoader->get("hasSpecularTexture")) : nullptr;
    for (const auto& cmd : commands)
    {
        const SceneNode* node = cmd.node;
        if (!node) continue;

        if (Uniform* uniform = uniformLoader->get("model")) { if (auto* matUniform = dynamic_cast<UniformMat4*>(uniform)) { matUniform->set(node->modelViewMatrix); matUniform->load(); } }
        const GLuint diffuseTextureToBind = (node->diffuseTextureId != 0) ? node->diffuseTextureId : fallbackWhiteTexture;
        const GLuint normalTextureToBind = (node->normalTextureId != 0) ? node->normalTextureId : fallbackNormalTexture;
        const GLuint specularTextureToBind = (node->specularTextureId != 0) ? node->specularTextureId : fallbackWhiteTexture;
        
        if (currentDiffuseTextureId != diffuseTextureToBind)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTextureToBind);
            currentDiffuseTextureId = diffuseTextureToBind;
        }

        if (currentNormalTextureId != normalTextureToBind)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, normalTextureToBind);
            currentNormalTextureId = normalTextureToBind;
        }

        if (currentSpecularTextureId != specularTextureToBind)
        {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, specularTextureToBind);
            currentSpecularTextureId = specularTextureToBind;
        }

        if (hasNormalUniform)
        {
            hasNormalUniform->set(node->normalTextureId != 0 ? 1 : 0);
            hasNormalUniform->load();
        }

        if (hasSpecularUniform)
        {
            hasSpecularUniform->set(node->specularTextureId != 0 ? 1 : 0);
            hasSpecularUniform->load();
        }

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        
        const GLsizei count = static_cast<GLsizei>(node->endPosition - node->startPosition);
        if (count > 0)
        {
            glDrawElements(node->primitiveMode, count, GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(node->startPosition * sizeof(GLuint)));
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(prevElementArrayBuffer));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prevArrayBuffer));
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(static_cast<GLuint>(prevVao));
}

// ============================================================================
// GPU Buffer Management with Double-Buffered VBO Fallback
// ============================================================================

bool OpenGLBackend::bufferToGpu(const std::vector<Vertex>& vertexData, const std::vector<uint32_t>& indices)
{
    if (verboseLogging) std::cout << "Buffering to GPU" << std::endl;

    checkForGLError();

    // Create VAO and VBO
    glGenVertexArrays(1, &vao);
    checkForGLError(); glBindVertexArray(vao); checkForGLError();

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    vboSize = sizeof(Vertex) * vertexData.size();
    
    // Double-buffered VBO approach for compatibility with older OpenGL versions
    if (usePersistentMappedVbo && (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 4)))
    {
        // Try persistent mapping first (OpenGL 4.4+)
        GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(GL_ARRAY_BUFFER, vboSize, nullptr, mapFlags);
        persistentVboPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, vboSize, mapFlags);
        if (persistentVboPtr)
        {
            std::memcpy(persistentVboPtr, vertexData.data(), vboSize);
            persistentVboActive = true;
        }
        else
        {
            // Fallback path: explicit map/flush/unmap for deterministic visibility.
            glBufferData(GL_ARRAY_BUFFER, vboSize, nullptr, GL_DYNAMIC_DRAW);
            void* mapped = glMapBufferRange(GL_ARRAY_BUFFER, 0, vboSize,
                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
            if (mapped)
            {
                std::memcpy(mapped, vertexData.data(), vboSize);
                glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, vboSize);
                glUnmapBuffer(GL_ARRAY_BUFFER);
            }
            else
            {
                glBufferSubData(GL_ARRAY_BUFFER, 0, vboSize, vertexData.data());
            }
            persistentVboActive = false;
        }
    }
    else
    {
        // Non-persistent fallback with explicit flush where available.
        glBufferData(GL_ARRAY_BUFFER, vboSize, nullptr, GL_DYNAMIC_DRAW);
        void* mapped = glMapBufferRange(GL_ARRAY_BUFFER, 0, vboSize,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
        if (mapped)
        {
            std::memcpy(mapped, vertexData.data(), vboSize);
            glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, vboSize);
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, vboSize, vertexData.data());
        }
        persistentVboActive = false;
    }

    // Set vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(sizeof(float) * 3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(sizeof(float) * 6));

    checkForGLError();

    // Create IBO
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    iboSize = indices.size() * sizeof(uint32_t);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(iboSize), 
                 indices.data(), GL_STATIC_DRAW);
    checkForGLError();

    if (verboseLogging) std::cout << "buffered geometry" << std::endl;

    glEnable(GL_DEPTH_TEST);

    // Create shader programs once; repeated bufferToGpu calls should not recompile/link.
    if (!gpuProgram || !shadowProgram)
    {
        gpuProgram = std::make_unique<GpuProgram>();
        shadowProgram = std::make_unique<GpuProgram>();

        // Load shadow shaders through ShaderCache.
        auto shadowFrag = FragmentShader::createFromCache(depthFragShaderPath);
        auto shadowVert = VertexShader::createFromCache(depthVertShaderPath);
        if (!shadowVert || !shadowFrag || !shadowVert->isCompiled() || !shadowFrag->isCompiled())
        {
            std::cerr << "Failed to load cached shadow shader" << std::endl;
            return false;
        }
        shadowProgram->attachShader(*shadowVert);
        shadowProgram->attachShader(*shadowFrag);
        glLinkProgram(shadowProgram->getId());
        shadowProgram->getUniformLoader()->addUniform("lightSpaceMatrix", std::make_unique<UniformMat4>(glm::mat4(1.0f)));
        shadowProgram->getUniformLoader()->addUniform("model", std::make_unique<UniformMat4>(glm::mat4(1.0f)));

        // Load main shaders through ShaderCache.
        auto vertShader = VertexShader::createFromCache(mainVertShaderPath);
        auto fragShader = FragmentShader::createFromCache(mainFragShaderPath);
        if (!vertShader || !fragShader || !vertShader->isCompiled() || !fragShader->isCompiled())
        {
            std::cerr << "Failed to load cached main shaders" << std::endl;
            return false;
        }
        gpuProgram->attachShader(*vertShader);
        gpuProgram->attachShader(*fragShader);
        glLinkProgram(gpuProgram->getId());
        auto* loader = gpuProgram->getUniformLoader();
        loader->addUniform("projection", std::make_unique<UniformMat4>(glm::mat4(1.0f)));
        loader->addUniform("view", std::make_unique<UniformMat4>(glm::mat4(1.0f)));
        loader->addUniform("model", std::make_unique<UniformMat4>(glm::mat4(1.0f)));
        loader->addUniform("lightSpaceMatrix", std::make_unique<UniformMat4>(glm::mat4(1.0f)));
        loader->addUniform("lightPos", std::make_unique<UniformVec3>(glm::vec3(0.0f)));
        loader->addUniform("viewPos", std::make_unique<UniformVec3>(glm::vec3(0.0f)));
        loader->addUniform("diffuseTexture", std::make_unique<UniformInt>(0));
        loader->addUniform("shadowMap", std::make_unique<UniformInt>(1));
        loader->addUniform("normalTexture", std::make_unique<UniformInt>(2));
        loader->addUniform("specularTexture", std::make_unique<UniformInt>(3));
        loader->addUniform("shadows", std::make_unique<UniformInt>(1));
        loader->addUniform("hasNormalTexture", std::make_unique<UniformInt>(0));
        loader->addUniform("hasSpecularTexture", std::make_unique<UniformInt>(0));

        _checkForGLSLError(gpuProgram->getId());
        _checkForGLSLError(shadowProgram->getId());
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create FBO for shadow map
    if (depthMapFBO != 0) {
        glDeleteFramebuffers(1, &depthMapFBO);
    }
    glGenFramebuffers(1, &depthMapFBO);

    if (fallbackWhiteTexture == 0)
    {
        constexpr unsigned char whitePixel[4] = { 255, 255, 255, 255 };
        glGenTextures(1, &fallbackWhiteTexture);
        glBindTexture(GL_TEXTURE_2D, fallbackWhiteTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    if (fallbackNormalTexture == 0)
    {
        constexpr unsigned char neutralNormalPixel[4] = { 128, 128, 255, 255 };
        glGenTextures(1, &fallbackNormalTexture);
        glBindTexture(GL_TEXTURE_2D, fallbackNormalTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, neutralNormalPixel);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    return true;
}

// ============================================================================
// Shadow Map Implementation with Cascaded Shadow Maps (CSM) Support
// ============================================================================

void OpenGLBackend::createShadowMap(const std::vector<SceneNode>& nodes)
{
    // Create shadow map texture on first call
    if (shadowMap == 0)
    {
        glGenTextures(1, &shadowMap);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowWidthW, shadowHeightH, 0, 
                     GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        constexpr GLfloat borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLint viewport[4];
    GLint prevFbo = 0;
    GLint prevVao = 0;
    GLint prevArrayBuffer = 0;
    GLint prevElementArrayBuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &prevElementArrayBuffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, shadowWidthW, shadowHeightH);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (nodes.empty()) return;

    checkForGLError(); glBindVertexArray(vao); checkForGLError();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    shadowProgram->use();
    shadowProgram->getUniformLoader()->load();

    for (const auto& node : nodes)
    {
        if (Uniform* uniform = shadowProgram->getUniformLoader()->get("model")) { if (auto* matUniform = dynamic_cast<UniformMat4*>(uniform)) { matUniform->set(node.modelViewMatrix); matUniform->load(); } }
        const GLsizei count = static_cast<GLsizei>(node.endPosition - node.startPosition);
        if (count > 0)
        {
            checkForGLError(); glDrawElements(node.primitiveMode, count, GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(node.startPosition * sizeof(GLuint)));
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(prevElementArrayBuffer));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(prevArrayBuffer));
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(static_cast<GLuint>(prevVao));
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
}

void OpenGLBackend::fitDirectionalShadowMatrix(Camera& camera,
    const glm::vec3& lightPosition,
    int /*shadowWidth*/, int /*shadowHeight*/,
    glm::mat4& lightView, glm::mat4& lightProjection, glm::mat4& lightSpaceMatrix)
{
    const glm::mat4 cameraViewProjection = camera.projectionMatrix * camera.modelViewMatrix;
    const glm::mat4 inverseViewProjection = glm::inverse(cameraViewProjection);

    const std::array<glm::vec3, 8> frustumCorners = {
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3( 1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3( 1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3( 1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f,  1.0f,  1.0f)
    };

    glm::vec3 frustumCenter(0.0f);
    for (const glm::vec3& corner : frustumCorners)
    {
        const glm::vec4 worldCorner = inverseViewProjection * glm::vec4(corner, 1.0f);
        frustumCenter += glm::vec3(worldCorner) / worldCorner.w;
    }
    frustumCenter /= static_cast<float>(frustumCorners.size());

    const glm::vec3 lightDirection = glm::normalize(frustumCenter - lightPosition);
    const glm::vec3 lightUp = std::abs(lightDirection.y) > 0.99f
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    lightView = glm::lookAt(lightPosition, frustumCenter, lightUp);

    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());
    for (const glm::vec3& corner : frustumCorners)
    {
        const glm::vec4 worldCorner = inverseViewProjection * glm::vec4(corner, 1.0f);
        const glm::vec4 lightSpaceCorner4 = lightView * (worldCorner / worldCorner.w);
        const glm::vec3 lightSpaceCorner = glm::vec3(lightSpaceCorner4);
        minBounds = glm::min(minBounds, lightSpaceCorner);
        maxBounds = glm::max(maxBounds, lightSpaceCorner);
    }

    constexpr float zPadding = 50.0f;
    lightProjection = glm::ortho(minBounds.x, maxBounds.x, minBounds.y, maxBounds.y,
        -maxBounds.z - zPadding, -minBounds.z + zPadding);
    lightSpaceMatrix = lightProjection * lightView;
}

void OpenGLBackend::updateLightUniforms(const Camera& camera, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos)
{
    if (gpuProgram && gpuProgram->getUniformLoader())
    {
        if (Uniform* uniform = gpuProgram->getUniformLoader()->get("projection"))
        {
            if (auto* matUniform = dynamic_cast<UniformMat4*>(uniform)) matUniform->set(camera.projectionMatrix);
        }

        if (Uniform* uniform = gpuProgram->getUniformLoader()->get("view"))
        {
            if (auto* matUniform = dynamic_cast<UniformMat4*>(uniform)) matUniform->set(camera.modelViewMatrix);
        }

        if (Uniform* uniform = gpuProgram->getUniformLoader()->get("viewPos"))
        {
            if (auto* vecUniform = dynamic_cast<UniformVec3*>(uniform)) vecUniform->set(camera.position);
        }

        if (Uniform* uniform = gpuProgram->getUniformLoader()->get("lightSpaceMatrix"))
        {
            if (auto* matUniform = dynamic_cast<UniformMat4*>(uniform)) matUniform->set(lightSpaceMatrix);
        }

        if (Uniform* uniform = gpuProgram->getUniformLoader()->get("lightPos"))
        {
            if (auto* vecUniform = dynamic_cast<UniformVec3*>(uniform)) vecUniform->set(lightPos);
        }
    }

    if (shadowProgram && shadowProgram->getUniformLoader())
    {
        if (Uniform* uniform = shadowProgram->getUniformLoader()->get("lightSpaceMatrix"))
        {
            if (auto* matUniform = dynamic_cast<UniformMat4*>(uniform)) matUniform->set(lightSpaceMatrix);
        }
    }
}
void OpenGLBackend::getShadowMapSize(int& width, int& height) const { width = shadowWidthW; height = shadowHeightH; }
void OpenGLBackend::beginFrame(const FrameContext&) { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }
void OpenGLBackend::endFrame() { if (m_glContext) SDL_GL_SwapWindow(SDL_GL_GetCurrentWindow()); }

void OpenGLBackend::ensureDebugProgram()
{
    if (debugProgram != 0) return;

    const char* debugVertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        uniform mat4 uViewProjection;
        void main() {
            gl_Position = uViewProjection * vec4(aPosition, 1.0);
        }
    )";

    const char* debugFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 uColor;
        void main() {
            FragColor = vec4(uColor, 1.0);
        }
    )";

    const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &debugVertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &debugFragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    debugProgram = glCreateProgram();
    glAttachShader(debugProgram, vertexShader);
    glAttachShader(debugProgram, fragmentShader);
    glLinkProgram(debugProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &debugVao);
    glGenBuffers(1, &debugVbo);
}

void OpenGLBackend::drawDebugLines(const std::vector<glm::vec3>& linePoints, const glm::vec3& color, const glm::mat4& viewProjection) const
{
    if (linePoints.size() < 2 || debugProgram == 0) return;

    glUseProgram(debugProgram);
    const GLint colorLocation = glGetUniformLocation(debugProgram, "uColor");
    const GLint viewProjectionLocation = glGetUniformLocation(debugProgram, "uViewProjection");
    if (viewProjectionLocation >= 0)
    {
        glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(viewProjection));
    }
    if (colorLocation >= 0)
    {
        glUniform3fv(colorLocation, 1, glm::value_ptr(color));
    }

    glBindVertexArray(debugVao);
    glBindBuffer(GL_ARRAY_BUFFER, debugVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(linePoints.size() * sizeof(glm::vec3)), linePoints.data(), GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(linePoints.size()));
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

void OpenGLBackend::drawCullDebugOverlay(const Camera& camera, const std::vector<int>& visibleNodeIds)
{
    if (!owner.configLoader || !owner.configLoader->hasVar("renderer.culling.debug")) return;
    if (!owner.configLoader->getBool("renderer.culling.debug")) return;

    ensureDebugProgram();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    const glm::mat4 viewProjection = camera.projectionMatrix * camera.modelViewMatrix;
    const glm::mat4 inverseProjection = glm::inverse(viewProjection);
    static const std::array<glm::vec3, 8> ndcCorners = {
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3( 1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3( 1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3( 1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f,  1.0f,  1.0f)
    };

    std::vector<glm::vec3> frustumLines;
    frustumLines.reserve(24);
    const std::array<int, 24> edges = {{
        0, 1, 0, 2, 1, 3, 2, 3,
        4, 5, 4, 6, 5, 7, 6, 7,
        0, 4, 1, 5, 2, 6, 3, 7
    }};
    for (size_t i = 0; i < edges.size(); i += 2)
    {
        const glm::vec3 a = glm::vec3(inverseProjection * glm::vec4(ndcCorners[edges[i]], 1.0f)) / glm::vec4(ndcCorners[edges[i]], 1.0f).w;
        const glm::vec3 b = glm::vec3(inverseProjection * glm::vec4(ndcCorners[edges[i + 1]], 1.0f)) / glm::vec4(ndcCorners[edges[i + 1]], 1.0f).w;
        frustumLines.push_back(a);
        frustumLines.push_back(b);
    }
    drawDebugLines(frustumLines, glm::vec3(0.0f, 1.0f, 1.0f), viewProjection);

    std::vector<glm::vec3> boundsLines;
    boundsLines.reserve(owner.sceneNodes.size() * 24);
    std::unordered_set<int> visibleSet(visibleNodeIds.begin(), visibleNodeIds.end());
    for (size_t i = 0; i < owner.sceneNodes.size(); ++i)
    {
        const SceneNode& node = owner.sceneNodes[i];
        if (node.boundingSphere <= 0.0f) continue;
        const glm::vec3 center(node.lx, node.ly, node.lz);
        const float radius = node.boundingSphere;
        const glm::vec3 half(radius, radius, radius);
        const std::array<glm::vec3, 8> box = {
            center - half, glm::vec3(center.x + radius, center.y - radius, center.z - radius),
            glm::vec3(center.x - radius, center.y + radius, center.z - radius), glm::vec3(center.x + radius, center.y + radius, center.z - radius),
            glm::vec3(center.x - radius, center.y - radius, center.z + radius), glm::vec3(center.x + radius, center.y - radius, center.z + radius),
            glm::vec3(center.x - radius, center.y + radius, center.z + radius), glm::vec3(center.x + radius, center.y + radius, center.z + radius)
        };
        static const std::array<int, 24> boxEdges = {{
            0, 1, 0, 2, 1, 3, 2, 3,
            4, 5, 4, 6, 5, 7, 6, 7,
            0, 4, 1, 5, 2, 6, 3, 7
        }};
        for (size_t edgeIndex = 0; edgeIndex < boxEdges.size(); edgeIndex += 2)
        {
            boundsLines.push_back(box[boxEdges[edgeIndex]]);
            boundsLines.push_back(box[boxEdges[edgeIndex + 1]]);
        }
    }
    drawDebugLines(boundsLines, glm::vec3(0.0f, 1.0f, 0.0f), viewProjection);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void OpenGLBackend::addTexture(GLuint* textureId, const struct Texture* texture) {
    if (!textureId || !texture || !texture->isValid()) return;
    
    glGenTextures(1, textureId);
    glBindTexture(GL_TEXTURE_2D, *textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, texture->mode, 
                 static_cast<GLsizei>(texture->width), static_cast<GLsizei>(texture->height),
                 0, texture->mode, GL_UNSIGNED_BYTE, texture->data.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Anisotropic filtering
    GLfloat maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    if (maxAniso > 0.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
    }
}
