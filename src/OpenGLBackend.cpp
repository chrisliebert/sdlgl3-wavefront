#include "OpenGLBackend.h"
#include "ConfigLoader.h"
#include "Camera.h"
#include "Renderer.h"
#include <iostream>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstring>
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
    const auto& cfg = getConfig();
    usePersistentMappedVbo = cfg.hasVar("renderer.vbo.persistent") ? cfg.getBool("renderer.vbo.persistent") : false;
    
    shadowWidthW = cfg.getInt("shadow.width");
    shadowHeightH = cfg.getInt("shadow.height");
}

OpenGLBackend::~OpenGLBackend()
{
}

ConfigLoader& OpenGLBackend::getConfig()
{
    return *owner.configLoader;
}

bool OpenGLBackend::initialize(SDL_Window* w)
{
    window = w;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    const auto& cfg = getConfig();
    if (cfg.getBool("cullFace")) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    
    glClearColor(1.0f, 0.8f, 0.8f, 1.0f);
    return true;
}

void OpenGLBackend::shutdown()
{
    // Delete OpenGL textures for scene nodes
    for (const auto& node : owner.sceneNodes)
    {
        if (node.diffuseTextureId != 0)
        {
            glDeleteTextures(1, &node.diffuseTextureId);
        }
    }

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
}

void OpenGLBackend::beginFrame(const FrameContext& frameContext)
{
    (void)frameContext;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLBackend::submit(const std::vector<RenderCommand>& commands)
{
    if (owner.sceneNodes.empty()) return;

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    
    if (!gpuProgram) {
        fprintf(stderr, "GPU program not initialized\n");
        return;
    }
    
    gpuProgram->use();
    gpuProgram->getUniformLoader()->load();

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

        if (owner.getShadowsEnabled())
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowMap);
        }

        const GLsizei count = static_cast<GLsizei>(node->endPosition - node->startPosition);
        if (count > 0)
        {
            glDrawElements(node->primitiveMode, count, GL_UNSIGNED_INT,
                reinterpret_cast<const void*>(node->startPosition * sizeof(GLuint)));
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(0);
}

void OpenGLBackend::endFrame()
{
    if (window) SDL_GL_SwapWindow(window);
}

// ============================================================================
// GPU Buffer Management with Double-Buffered VBO Fallback
// ============================================================================

bool OpenGLBackend::bufferToGpu(Camera& camera, std::string_view /*cacheFilename*/, bool /*loadCachedScene*/)
{
    const auto& cfg = getConfig();
    if (cfg.getBool("renderer.verbose")) std::cout << "Buffering to GPU" << std::endl;

    // Load textures for scene nodes
    for (size_t i = 0; i < owner.sceneNodes.size(); ++i)
    {
        const char* materialName = owner.sceneNodes[i].material;
        auto matIt = owner.materials.find(materialName);
        if (matIt == owner.materials.end())
        {
            std::cerr << "Material " << materialName << " was not loaded" << std::endl;
        }
        else if (std::strlen(matIt->second.diffuseTexName) > 0)
        {
            if (cfg.getBool("renderer.verbose"))
            {
                std::cout << "Texture stage [" << i << "] diffuse: " << matIt->second.diffuseTexName << std::endl;
            }
            owner.addTexture(matIt->second.diffuseTexName, &owner.sceneNodes[i].diffuseTextureId);
        }

        if (matIt != owner.materials.end() && std::strlen(matIt->second.normalTexName) > 0)
        {
            if (cfg.getBool("renderer.verbose"))
            {
                std::cout << "Texture stage [" << i << "] normal: " << matIt->second.normalTexName << std::endl;
            }
            owner.addTexture(matIt->second.normalTexName, &owner.sceneNodes[i].normalTextureId);
        }

        if (matIt != owner.materials.end() && std::strlen(matIt->second.specularTexName) > 0)
        {
            if (cfg.getBool("renderer.verbose"))
            {
                std::cout << "Texture stage [" << i << "] specular: " << matIt->second.specularTexName << std::endl;
            }
            owner.addTexture(matIt->second.specularTexName, &owner.sceneNodes[i].specularTextureId);
        }
    }

    if (cfg.getBool("renderer.verbose")) std::cout << "Texture stage complete" << std::endl;

    checkForGLError();

    // Create VAO and VBO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    const size_t vertexBytes = sizeof(Vertex) * owner.vertexData.size();
    
    // Double-buffered VBO approach for compatibility with older OpenGL versions
    if (usePersistentMappedVbo && (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 4)))
    {
        // Try persistent mapping first (OpenGL 4.4+)
        GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glBufferStorage(GL_ARRAY_BUFFER, vertexBytes, nullptr, mapFlags);
        persistentVboPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertexBytes, mapFlags);
        if (persistentVboPtr)
        {
            std::memcpy(persistentVboPtr, owner.vertexData.data(), vertexBytes);
            persistentVboActive = true;
        }
        else
        {
            // Fallback to invalidate-and-replace approach
            glBufferData(GL_ARRAY_BUFFER, vertexBytes, owner.vertexData.data(), GL_DYNAMIC_DRAW);
            persistentVboActive = false;
        }
    }
    else
    {
        // Double-buffered approach for OpenGL 3.0+
        glBufferData(GL_ARRAY_BUFFER, vertexBytes, owner.vertexData.data(), GL_DYNAMIC_DRAW);
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(owner.indices.size() * sizeof(GLuint)), 
                 owner.indices.data(), GL_STATIC_DRAW);
    checkForGLError();

    if (cfg.getBool("renderer.verbose")) std::cout << "buffered geometry" << std::endl;

    glEnable(GL_DEPTH_TEST);

    // Create shader programs with caching support
    gpuProgram = std::make_unique<GpuProgram>();
    shadowProgram = std::make_unique<GpuProgram>();

    // Load shadow shaders
    std::string shadowVertPath;
    std::string shadowFragPath;
    { const std::string _sd(SHADER_DIRECTORY); const std::string _ds(DIRECTORY_SEPARATOR); const std::string _sv(cfg.getVar("shader.depth.vert")); shadowVertPath = _sd + _ds + _sv; }
    { const std::string _sd(SHADER_DIRECTORY); const std::string _ds(DIRECTORY_SEPARATOR); const std::string _sf(cfg.getVar("shader.depth.frag")); shadowFragPath = _sd + _ds + _sf; }
    
    VertexShader shadowVert(shadowVertPath);
    FragmentShader shadowFrag(shadowFragPath);
    shadowProgram->attachShader(shadowVert);
    shadowProgram->attachShader(shadowFrag);
    glLinkProgram(shadowProgram->getId());

    // Load main shaders
    std::string vertPath(std::string(SHADER_DIRECTORY) + std::string(DIRECTORY_SEPARATOR) + std::string(cfg.getVar("shader.vert")));
    std::string fragPath(std::string(SHADER_DIRECTORY) + std::string(DIRECTORY_SEPARATOR) + std::string(cfg.getVar("shader.frag")));
    
    VertexShader vertShader(vertPath);
    FragmentShader fragShader(fragPath);
    gpuProgram->attachShader(vertShader);
    gpuProgram->attachShader(fragShader);
    glLinkProgram(gpuProgram->getId());

    _checkForGLSLError(gpuProgram->getId());
    _checkForGLSLError(shadowProgram->getId());

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Create FBO for shadow map
    if (depthMapFBO != 0) {
        glDeleteFramebuffers(1, &depthMapFBO);
    }
    glGenFramebuffers(1, &depthMapFBO);

    // Set uniforms with smart pointers
    // Fix: Use consistent light position matching Renderer::render() (was 0,100,0, now 10,50,0)
    const glm::vec3 lightPos = camera.position + Math::getLightPositionOffset();
    const glm::mat4 lightProjection = glm::ortho(
        cfg.getFloat("shadow.ortho.left"), cfg.getFloat("shadow.ortho.right"),
        cfg.getFloat("shadow.ortho.bottom"), cfg.getFloat("shadow.ortho.top"),
        cfg.getFloat("shadow.ortho.near"), cfg.getFloat("shadow.ortho.far"));
    const glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    const glm::mat4 identityModel(1.0f);

    shadowProgram->getUniformLoader()->addUniform("lightSpaceMatrix", std::make_unique<UniformMat4>(lightSpaceMatrix));
    shadowProgram->getUniformLoader()->addUniform("model", std::make_unique<UniformMat4>(identityModel));

    gpuProgram->getUniformLoader()->addUniform("projection", std::make_unique<UniformMat4>(camera.projectionMatrix));
    gpuProgram->getUniformLoader()->addUniform("view", std::make_unique<UniformMat4>(camera.modelViewMatrix));
    gpuProgram->getUniformLoader()->addUniform("model", std::make_unique<UniformMat4>(identityModel));
    gpuProgram->getUniformLoader()->addUniform("lightSpaceMatrix", std::make_unique<UniformMat4>(lightSpaceMatrix));
    gpuProgram->getUniformLoader()->addUniform("lightPos", std::make_unique<UniformVec3>(lightPos));
    gpuProgram->getUniformLoader()->addUniform("viewPos", std::make_unique<UniformVec3>(camera.position));
    gpuProgram->getUniformLoader()->addUniform("diffuseTexture", std::make_unique<UniformInt>(0));
    gpuProgram->getUniformLoader()->addUniform("shadowMap", std::make_unique<UniformInt>(1));
    gpuProgram->getUniformLoader()->addUniform("normalTexture", std::make_unique<UniformInt>(2));
    gpuProgram->getUniformLoader()->addUniform("specularTexture", std::make_unique<UniformInt>(3));
    gpuProgram->getUniformLoader()->addUniform("hasNormalTexture", std::make_unique<UniformInt>(0));
    gpuProgram->getUniformLoader()->addUniform("hasSpecularTexture", std::make_unique<UniformInt>(0));
    GLint _shadowsVal = owner.getShadowsEnabled() ? static_cast<GLint>(1) : static_cast<GLint>(0);
    gpuProgram->getUniformLoader()->addUniform("shadows", std::make_unique<UniformInt>(_shadowsVal));

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

GLuint OpenGLBackend::createShadowMap(Camera& camera)
{
    (void)camera;

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
    glGetIntegerv(GL_VIEWPORT, viewport);
    glViewport(0, 0, shadowWidthW, shadowHeightH);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    if (owner.sceneNodes.empty()) return 0;

    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    shadowProgram->use();
    shadowProgram->getUniformLoader()->load();

    for (size_t _ni = 0; _ni < owner.sceneNodes.size(); ++_ni)
    {
        const auto& node = owner.sceneNodes[_ni];
        const GLsizei count = static_cast<GLsizei>(node.endPosition - node.startPosition);
        if (count > 0)
        {
            glDrawRangeElementsBaseVertex(node.primitiveMode, node.startPosition, node.endPosition,
                count, GL_UNSIGNED_INT, reinterpret_cast<const void*>(0), static_cast<GLint>(node.startPosition));
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindTexture(GL_TEXTURE_2D, shadowMap);

    return shadowMap;
}

void OpenGLBackend::updateShadowMap(Camera& camera)
{
    // Create or recreate the shadow map
    createShadowMap(camera);
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
