#include "Renderer.h"
#include "ConfigLoader.h"
#include "OpenGLBackend.h"
#include "PathUtil.h"
#include "MathUtil.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <filesystem>
#include <limits>
#include <algorithm>
#include <cctype>
#include <map>



void checkForGLSLError(GLuint programId)
{
    GLint result = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &result);
    if (result == GL_FALSE)
    {
        GLint logLength = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            std::vector<GLchar> log(static_cast<std::size_t>(logLength), 0);
            glGetProgramInfoLog(programId, static_cast<GLsizei>(log.size()), nullptr, log.data());
            fprintf(stdout, "Shader link error: %s\n", log.data());
        }
    }
}

#ifdef GLAD_DEBUG
void pre_gl_call(const char* name, void* funcptr, int len_args, ...)
{
    (void)name; (void)funcptr; (void)len_args;
    std::cout << "Calling: " << name << " (" << len_args << " arguments)" << std::endl;
}
#endif

Renderer::Renderer()
{
    configLoader = std::make_unique<ConfigLoader>("renderer.cfg");
    
    shadowsEnabled = configLoader->getBool("shadow.enabled");
    binCacheWriterThread = nullptr;
    profilerEnabled = configLoader->hasVar("renderer.profileHud") ? configLoader->getBool("renderer.profileHud") : false;
    hierarchicalCullingEnabled = configLoader->hasVar("renderer.culling.hierarchical") ? configLoader->getBool("renderer.culling.hierarchical") : true;
    occlusionCullingEnabled = configLoader->hasVar("renderer.culling.occlusion") ? configLoader->getBool("renderer.culling.occlusion") : true;
    cullLeafSize = configLoader->hasVar("renderer.culling.leafSize") ? configLoader->getInt("renderer.culling.leafSize") : 16;
    occlusionRetestFrames = configLoader->hasVar("renderer.culling.occlusionRetestFrames") ? configLoader->getInt("renderer.culling.occlusionRetestFrames") : 8;
    occlusionMinSamples = configLoader->hasVar("renderer.culling.occlusionMinSamples") ? configLoader->getInt("renderer.culling.occlusionMinSamples") : 1;
    frustumCullingEnabled = configLoader->hasVar("renderer.culling.frustum") ? configLoader->getBool("renderer.culling.frustum") : true;

}

Renderer::~Renderer()
{
    // Wait for cache writer thread to exit (thread-safe cleanup)
    if (binCacheWriterThread != nullptr)
    {
        int status = 0;
        SDL_WaitThread(static_cast<SDL_Thread*>(binCacheWriterThread), &status);
        binCacheWriterThread = nullptr;
    }

    // Delete OpenGL resources (thread-safe via renderer destructor)
    if (!occlusionQueries.empty())
    {
        glDeleteQueries(static_cast<GLsizei>(occlusionQueries.size()), occlusionQueries.data());
    }

    if (shadowMap != 0)
    {
        glDeleteTextures(1, &shadowMap);
        shadowMap = 0;
    }

    if (backend)
    {
        backend->shutdown();
    }

}

void Renderer::addMaterial(std::string_view name, const Material& material)
{
    std::lock_guard<std::mutex> lock(sceneDataMutex);
    materials.emplace(std::string(name), material);
}

void Renderer::addSceneNode(SceneNode node)
{
    std::lock_guard<std::mutex> lock(sceneDataMutex);
    sceneNodes.push_back(std::move(node));
    ++sceneRevision;
    shadowDirty = true;
}

namespace {
    int getTextureMode(SDL_Surface* image)
    {
        int bpp = 0;
            bpp = static_cast<int>(SDL_BYTESPERPIXEL(image->format));
        return (bpp == 4) ? GL_RGBA : GL_RGB;
    }

    std::string toLowerAscii(std::string value)
    {
        for (char& c : value)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return value;
    }

    std::string normalizeTextureRef(std::string_view textureRef)
    {
        std::string normalized(textureRef);
        if (normalized.empty()) return normalized;

        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        while (normalized.rfind("./", 0) == 0)
        {
            normalized.erase(0, 2);
        }

        const std::string lower = toLowerAscii(normalized);
        if (lower.rfind("textures/", 0) == 0)
        {
            normalized.erase(0, 9);
        }

        return normalized;
    }

    std::filesystem::path resolveExistingTexturePath(std::string_view textureRef)
    {
        const std::string normalized = normalizeTextureRef(textureRef);
        if (normalized.empty()) return {};

        const std::array<std::filesystem::path, 3> candidates = {
            std::filesystem::path(normalized),
            std::filesystem::path(std::filesystem::path(normalized).filename().string()),
            std::filesystem::path(textureRef)
        };

        std::error_code ec;
        for (const auto& candidate : candidates)
        {
            if (candidate.empty()) continue;

            const std::filesystem::path secureTexturePath = resolveSecurePath(TEXTURE_DIRECTORY, candidate);
            if (!secureTexturePath.empty() && std::filesystem::exists(secureTexturePath, ec) && !ec)
            {
                return secureTexturePath;
            }

            const std::filesystem::path secureModelPath = resolveSecurePath(MODEL_DIRECTORY, candidate);
            if (!secureModelPath.empty() && std::filesystem::exists(secureModelPath, ec) && !ec)
            {
                return secureModelPath;
            }
        }

        const std::filesystem::path basename = std::filesystem::path(normalized).filename();
        if (basename.empty()) return {};

        const std::string targetName = toLowerAscii(basename.string());
        for (const auto& entry : std::filesystem::recursive_directory_iterator(TEXTURE_DIRECTORY, std::filesystem::directory_options::skip_permission_denied, ec))
        {
            if (ec) break;
            if (!entry.is_regular_file()) continue;

            const std::string entryName = toLowerAscii(entry.path().filename().string());
            if (entryName == targetName)
            {
                return entry.path();
            }
        }

        return {};
    }
}

std::shared_ptr<Texture> Renderer::textureFromSurface(std::unique_ptr<SDL_Surface, SdlSurfaceDeleter> image)
{
    auto texture = std::make_shared<Texture>();
    texture->mode = getTextureMode(image.get());
    texture->width = static_cast<unsigned>(image->w);
    texture->height = static_cast<unsigned>(image->h);
    texture->bpp = static_cast<unsigned>(SDL_BYTESPERPIXEL(image->format));
    // Copy pixel data to owned buffer (prevents use-after-free)
    const size_t dataSize = static_cast<size_t>(image->w) * image->h * texture->bpp;
    if (dataSize > 0 && image->pixels)
    {
        texture->data = std::shared_ptr<unsigned char[]>(new unsigned char[dataSize], [](unsigned char* p) { delete[] p; });
        std::memcpy(texture->data.get(), image->pixels, dataSize);
    }
    return texture;
}

void Renderer::addTexture(std::string_view textureFileName, GLuint* textureId, std::unique_ptr<SDL_Surface, SdlSurfaceDeleter> image)
{
    if (!textureId) return;
    
    if (!image)
    {
        const std::string blankPath = (std::filesystem::path(TEXTURE_DIRECTORY) / "DEFAULT_BLANK_TEXTURE.png").string();
        
        auto it = textures.find(blankPath);
        if (it == textures.end())
        {
            std::unique_ptr<SDL_Surface, SdlSurfaceDeleter> defaultImage(IMG_Load(blankPath.c_str()));
            if (!defaultImage)
            {
                std::cerr << "Error loading default blank texture" << std::endl;
                return;
            }
            auto blankTexture = textureFromSurface(std::move(defaultImage));
            textures[blankPath] = blankTexture;
        }
    }
    
    if (image)
    {
        auto texture = textureFromSurface(std::move(image));
        textures[std::string(textureFileName)] = texture;
    }
    
    auto it = textures.find(std::string(textureFileName));
    if (it == textures.end()) return;
    
    const Texture* tex = it->second.get();
    if (!tex || !tex->isValid()) return;
    
    glGenTextures(1, textureId);
    glBindTexture(GL_TEXTURE_2D, *textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, tex->mode, 
                 static_cast<GLsizei>(tex->width), static_cast<GLsizei>(tex->height),
                 0, tex->mode, GL_UNSIGNED_BYTE, tex->data.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Renderer::addTexture(std::string_view /*textureFileName*/, GLuint* textureId, const Texture* texture)
{
    if (!textureId || !texture || !texture->isValid()) return;
    
    glGenTextures(1, textureId);
    glBindTexture(GL_TEXTURE_2D, *textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, texture->mode, 
                 static_cast<GLsizei>(texture->width), static_cast<GLsizei>(texture->height),
                 0, texture->mode, GL_UNSIGNED_BYTE, texture->data.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Renderer::addTexture(std::string_view textureFileName, GLuint* textureId)
{
    if (!textureId) return;
    
    auto it = textures.find(std::string(textureFileName));
    if (it != textures.end())
    {
        addTexture(textureFileName, textureId, it->second.get());
        return;
    }
    
    std::filesystem::path securePath = resolveExistingTexturePath(textureFileName);
    if (securePath.empty()) {
        std::cerr << "Texture not found: " << textureFileName << std::endl;
        return;
    }
    
    std::unique_ptr<SDL_Surface, SdlSurfaceDeleter> image(IMG_Load(securePath.string().c_str()));
    if (image)
    {
        addTexture(textureFileName, textureId, std::move(image));
    }
    else
    {
        std::cerr << "Failed to load texture image: " << securePath << " (" << SDL_GetError() << ")" << std::endl;
    }
}

void Renderer::setBackend(std::unique_ptr<IRenderBackend> newBackend)
{
    backend = std::move(newBackend);
}

void Renderer::addWavefront(std::string_view fileName, const glm::mat4& matrix)
{
    GLuint startPosition = 0;
    int skippedFaces = 0;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materialsList;
    
    std::filesystem::path requestedPath(fileName);
    requestedPath = requestedPath.lexically_normal();
    auto reqIt = requestedPath.begin();
    if (reqIt != requestedPath.end() && *reqIt == std::filesystem::path(MODEL_DIRECTORY))
    {
        ++reqIt;
        std::filesystem::path stripped;
        for (; reqIt != requestedPath.end(); ++reqIt)
        {
            stripped /= *reqIt;
        }
        requestedPath = stripped;
    }

    std::filesystem::path modelPath = resolveSecurePath(MODEL_DIRECTORY, requestedPath);
    if (modelPath.empty() || !std::filesystem::exists(modelPath))
    {
        std::cerr << "Model file not found or path is insecure: " << fileName << std::endl;
        return;
    }

    std::filesystem::path parentPath = modelPath.parent_path();
    std::string modelDirectory = parentPath.empty() ? std::string(".") : parentPath.lexically_normal().string();
    if (!modelDirectory.empty())
    {
        modelDirectory = std::filesystem::path(modelDirectory).lexically_normal().generic_string();
        if (!modelDirectory.empty() && modelDirectory.back() != '/')
        {
            modelDirectory.push_back('/');
        }
    }
    std::string fileNameStr = modelPath.lexically_normal().string();
    
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = modelDirectory;
    reader_config.triangulate = true;

    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(fileNameStr, reader_config))
    {
        if (!reader.Error().empty())
            std::cerr << "TinyObjReader: " << reader.Error() << std::endl;
        return;
    }
    if (!reader.Warning().empty())
        std::cout << "TinyObjReader warning: " << reader.Warning() << std::endl;

    attrib = reader.GetAttrib();
    shapes = reader.GetShapes();
    materialsList = reader.GetMaterials();

    if (attrib.vertices.empty() || shapes.empty())
    {
        std::cerr << "OBJ validation failed: no geometry found in " << fileName << std::endl;
        return;
    }

    static constexpr const char* kDefaultMaterialName = "__default__";
    if (materials.find(kDefaultMaterialName) == materials.end())
    {
        Material defaultMaterial{};
        defaultMaterial.setName(kDefaultMaterialName);
        addMaterial(kDefaultMaterialName, defaultMaterial);
    }

    // Load materials with proper string handling
    int materialCount = 0;
    int mapKdReferenced = 0;
    int diffuseResolved = 0;
    int derivedBaseColorCount = 0;
    int roughnessAsDiffuseCount = 0;
    int heuristicMappedCount = 0;
    int unresolvedDiffuseCount = 0;
    std::vector<std::string> unresolvedExamples;
    unresolvedExamples.reserve(8);

    for (size_t i = 0; i < materialsList.size(); ++i)
    {
        Material m{};
        ++materialCount;
        
        // Copy color values (ambient, diffuse, specular, emission are float[3] in material_t)
        if (materialsList[i].dummy == 0) { // Check if data is valid
            std::memcpy(&m.ambient, &materialsList[i].ambient[0], sizeof(float) * 3);
            std::memcpy(&m.diffuse, &materialsList[i].diffuse[0], sizeof(float) * 3);
            std::memcpy(&m.emission, &materialsList[i].emission[0], sizeof(float) * 3);
            std::memcpy(&m.specular, &materialsList[i].specular[0], sizeof(float) * 3);
            std::memcpy(&m.transmittance, &materialsList[i].transmittance[0], sizeof(float) * 3);
        }
        
        m.shininess = materialsList[i].shininess;
        m.ior = materialsList[i].ior;
        m.dissolve = materialsList[i].dissolve;
        m.illum = materialsList[i].illum;

        auto deriveBaseColorTexture = [](std::string_view sourceTexture) {
            if (sourceTexture.empty()) return std::string();

            std::string candidate(sourceTexture);
            const auto replaceAll = [](std::string& text, std::string_view from, std::string_view to) {
                size_t pos = 0;
                while ((pos = text.find(from, pos)) != std::string::npos)
                {
                    text.replace(pos, from.size(), to);
                    pos += to.size();
                }
            };

            replaceAll(candidate, "_Roughness", "_BaseColor");
            replaceAll(candidate, "_roughness", "_BaseColor");
            replaceAll(candidate, " Roughness", " BaseColor");
            return candidate;
        };

        auto deriveNormalTexture = [](std::string_view sourceTexture) {
            if (sourceTexture.empty()) return std::string();

            std::string candidate(sourceTexture);
            const auto replaceAll = [](std::string& text, std::string_view from, std::string_view to) {
                size_t pos = 0;
                while ((pos = text.find(from, pos)) != std::string::npos)
                {
                    text.replace(pos, from.size(), to);
                    pos += to.size();
                }
            };

            replaceAll(candidate, "_Roughness", "_Normal");
            replaceAll(candidate, "_roughness", "_Normal");
            replaceAll(candidate, " Roughness", " Normal");
            return candidate;
        };

        std::string diffuseTextureName = materialsList[i].diffuse_texname;
        const bool hadMapKd = !diffuseTextureName.empty();
        if (hadMapKd) ++mapKdReferenced;

        if (resolveExistingTexturePath(diffuseTextureName).empty())
        {
            diffuseTextureName.clear();

            const std::array<std::string, 2> roughnessLikeSources = {
                materialsList[i].roughness_texname,
                materialsList[i].specular_highlight_texname
            };

            for (const std::string& src : roughnessLikeSources)
            {
                if (src.empty()) continue;

                const std::string derivedBaseColor = deriveBaseColorTexture(src);
                if (!derivedBaseColor.empty() && !resolveExistingTexturePath(derivedBaseColor).empty())
                {
                    diffuseTextureName = derivedBaseColor;
                    ++derivedBaseColorCount;
                    break;
                }

                // Last-resort fallback: use the referenced texture directly as diffuse.
                if (!resolveExistingTexturePath(src).empty())
                {
                    diffuseTextureName = src;
                    ++roughnessAsDiffuseCount;
                    break;
                }
            }
        }

        std::string normalTextureName = materialsList[i].normal_texname;
        if (normalTextureName.empty())
        {
            normalTextureName = materialsList[i].bump_texname;
        }
        if (normalTextureName.empty())
        {
            const std::array<std::string, 2> roughnessLikeSources = {
                materialsList[i].roughness_texname,
                materialsList[i].specular_highlight_texname
            };
            for (const std::string& src : roughnessLikeSources)
            {
                if (src.empty()) continue;
                const std::string derivedNormal = deriveNormalTexture(src);
                if (!derivedNormal.empty() && !resolveExistingTexturePath(derivedNormal).empty())
                {
                    normalTextureName = derivedNormal;
                    break;
                }
            }
        }

        std::string specularTextureName = materialsList[i].specular_texname;
        if (specularTextureName.empty() && !materialsList[i].specular_highlight_texname.empty())
        {
            const std::string lower = toLowerAscii(materialsList[i].specular_highlight_texname);
            if (lower.find("roughness") == std::string::npos)
            {
                specularTextureName = materialsList[i].specular_highlight_texname;
            }
        }

        if (diffuseTextureName.empty())
        {
            const std::string materialNameLower = toLowerAscii(materialsList[i].name);

            auto tryAssignHeuristic = [&](std::string_view diffuseCandidate, std::string_view normalCandidate, std::string_view specularCandidate) {
                bool assigned = false;
                if (diffuseTextureName.empty() && !resolveExistingTexturePath(diffuseCandidate).empty())
                {
                    diffuseTextureName = std::string(diffuseCandidate);
                    assigned = true;
                }
                if (normalTextureName.empty() && !resolveExistingTexturePath(normalCandidate).empty())
                {
                    normalTextureName = std::string(normalCandidate);
                }
                if (specularTextureName.empty() && !resolveExistingTexturePath(specularCandidate).empty())
                {
                    specularTextureName = std::string(specularCandidate);
                }
                return assigned;
            };

            bool heuristicAssigned = false;
            if (materialNameLower.find("chair_wood") != std::string::npos || materialNameLower.find("woodsurface") != std::string::npos)
            {
                heuristicAssigned = tryAssignHeuristic(
                    "cloth_ball/cracked_wood_plank_diffuse_xtm.jpg",
                    "cloth_ball/cracked_wood_plank_normal_xtm.jpg",
                    "cloth_ball/cracked_wood_plank_specular_xtm.jpg");
            }
            else if (materialNameLower.find("chair_thickness") != std::string::npos || materialNameLower.find("towel") != std::string::npos)
            {
                heuristicAssigned = tryAssignHeuristic(
                    "cloth_ball/Striped_cotton_01_diffuse_xtm.png",
                    "cloth_ball/Striped_cotton_01_normal_xtm.png",
                    "");
            }
            else if (materialNameLower.find("water") != std::string::npos)
            {
                heuristicAssigned = tryAssignHeuristic(
                    "water/top.jpg",
                    "",
                    "");
            }
            else if (materialNameLower == "white")
            {
                heuristicAssigned = tryAssignHeuristic(
                    "bar/bar_Paint White_BaseColor.png",
                    "bar/bar_Paint White_Normal.png",
                    "");
            }

            if (heuristicAssigned)
            {
                ++heuristicMappedCount;
                std::cout << "Texture debug heuristic material map: " << materialsList[i].name
                          << " -> " << diffuseTextureName << std::endl;
            }
        }

        if (!diffuseTextureName.empty())
        {
            ++diffuseResolved;
        }
        else
        {
            ++unresolvedDiffuseCount;
            if (unresolvedExamples.size() < 8)
            {
                std::ostringstream msg;
                msg << materialsList[i].name << " (map_Kd='" << materialsList[i].diffuse_texname
                    << "', map_Ns='" << materialsList[i].specular_highlight_texname
                    << "', map_Pr='" << materialsList[i].roughness_texname << "')";
                unresolvedExamples.push_back(msg.str());
            }
        }

        m.setName(materialsList[i].name);
        m.setDiffuseTexName(diffuseTextureName);
        m.setNormalTexName(normalTextureName);
        m.setSpecularTexName(specularTextureName);

        addMaterial(materialsList[i].name, m);
    }

    std::cout << "Texture debug: materials=" << materialCount
              << ", map_Kd refs=" << mapKdReferenced
              << ", diffuse resolved=" << diffuseResolved
              << ", derived BaseColor=" << derivedBaseColorCount
              << ", roughness-as-diffuse=" << roughnessAsDiffuseCount
              << ", heuristic mapped=" << heuristicMappedCount
              << ", unresolved=" << unresolvedDiffuseCount
              << std::endl;
    for (const auto& example : unresolvedExamples)
    {
        std::cout << "Texture debug unresolved material: " << example << std::endl;
    }

    // Process shapes with thread-safe node addition
    for (size_t i = 0; i < shapes.size(); ++i)
    {
        std::vector<Vertex> localVertices;
        int currentMaterialId = -1;
        size_t indexOffset = 0;

        for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); ++f)
        {
            int faceMaterialId = -1;
            if (f < shapes[i].mesh.material_ids.size())
                faceMaterialId = shapes[i].mesh.material_ids[f];

            if (faceMaterialId < 0 || static_cast<size_t>(faceMaterialId) >= materialsList.size())
            {
                faceMaterialId = -1;
            }

            if (faceMaterialId != currentMaterialId) {
                if (!localVertices.empty()) {
                    SceneNode sceneNode;
                    sceneNode.setName(shapes[i].name);
                    if (currentMaterialId >= 0 && static_cast<size_t>(currentMaterialId) < materialsList.size()) {
                        sceneNode.setMaterial(materialsList[currentMaterialId].name);
                    } else {
                        sceneNode.setMaterial(kDefaultMaterialName);
                    }
                    sceneNode.vertexDataSize = localVertices.size();
                    sceneNode.vertexData = std::make_unique<Vertex[]>(sceneNode.vertexDataSize);
                    std::memcpy(sceneNode.vertexData.get(), localVertices.data(), sizeof(Vertex) * sceneNode.vertexDataSize);
                    sceneNode.setDrawRange(startPosition);
                    sceneNode.primitiveMode = GL_TRIANGLES;
                    sceneNode.diffuseTextureId = 0;
                    sceneNode.modelViewMatrix = matrix;
                    addSceneNode(std::move(sceneNode));
                    startPosition += static_cast<GLuint>(sceneNode.vertexDataSize);
                    localVertices.clear();
                }
                currentMaterialId = faceMaterialId;
            }

            size_t fv = shapes[i].mesh.num_face_vertices[f];
            bool faceValid = true;
            std::vector<Vertex> faceVertices;
            faceVertices.reserve(fv);

            if (fv != 3)
            {
                faceValid = false;
            }

            for (size_t v = 0; v < fv; ++v)
            {
                Vertex vert{};
                tinyobj::index_t idx = shapes[i].mesh.indices[indexOffset + v];

                if (idx.vertex_index < 0) {
                    faceValid = false;
                    break;
                }

                const size_t vertexBase = static_cast<size_t>(idx.vertex_index) * 3u;
                if (vertexBase + 2u >= attrib.vertices.size()) {
                    faceValid = false;
                    break;
                }

                vert.vertex[0] = attrib.vertices[vertexBase + 0u];
                vert.vertex[1] = attrib.vertices[vertexBase + 1u];
                vert.vertex[2] = attrib.vertices[vertexBase + 2u];

                if (idx.normal_index >= 0) {
                    const size_t normalBase = static_cast<size_t>(idx.normal_index) * 3u;
                    if (normalBase + 2u < attrib.normals.size()) {
                        vert.normal[0] = attrib.normals[normalBase + 0u];
                        vert.normal[1] = attrib.normals[normalBase + 1u];
                        vert.normal[2] = attrib.normals[normalBase + 2u];
                    }
                }

                if (idx.texcoord_index >= 0) {
                    const size_t texBase = static_cast<size_t>(idx.texcoord_index) * 2u;
                    if (texBase + 1u < attrib.texcoords.size()) {
                        vert.textureCoordinate[0] = attrib.texcoords[texBase + 0u];
                        vert.textureCoordinate[1] = 1.0f - attrib.texcoords[texBase + 1u];
                    }
                }
                faceVertices.push_back(vert);
            }

            if (faceValid) {
                localVertices.insert(localVertices.end(), faceVertices.begin(), faceVertices.end());
            } else {
                skippedFaces++;
            }

            indexOffset += fv;
        }

        if (!localVertices.empty())
        {
            SceneNode sceneNode;
            sceneNode.setName(shapes[i].name);
            if (currentMaterialId >= 0 && static_cast<size_t>(currentMaterialId) < materialsList.size()) {
                sceneNode.setMaterial(materialsList[currentMaterialId].name);
            } else {
                sceneNode.setMaterial(kDefaultMaterialName);
            }
            sceneNode.vertexDataSize = localVertices.size();
            sceneNode.vertexData = std::make_unique<Vertex[]>(sceneNode.vertexDataSize);
            std::memcpy(sceneNode.vertexData.get(), localVertices.data(), sizeof(Vertex) * sceneNode.vertexDataSize);
            sceneNode.setDrawRange(startPosition);
            sceneNode.primitiveMode = GL_TRIANGLES;
            sceneNode.diffuseTextureId = 0;
            sceneNode.modelViewMatrix = matrix;
            addSceneNode(std::move(sceneNode));
            startPosition += static_cast<GLuint>(sceneNode.vertexDataSize);
        }
    }

    if (skippedFaces > 0)
    {
        std::cerr << "Skipped " << skippedFaces << " invalid face(s) while loading " << fileName << std::endl;
    }
}

// ============================================================================
// Binary Cache with Versioning (Fix #7.2)
// ============================================================================

struct BinCacheFileHeader {
    uint32_t magic = 0x5741564F; // "OWAV" - Wavefront cache magic number
    uint32_t version = BINARY_CACHE_VERSION;
    size_t numMaterials;
    size_t numSceneNodes;
    size_t numVertices;
    size_t numTextures;
};

int Renderer::createBinCacheInternal()
{
    std::lock_guard<std::mutex> lock(sceneDataMutex);
    const char* filename = cacheFileName.c_str();
    
    std::ofstream binFile(filename, std::ios::binary | std::ios::trunc);
    if (!binFile.is_open())
    {
        std::cerr << "Unable to open " << filename << " for writing" << std::endl;
        return -1;
    }

    BinCacheFileHeader header{};
    header.numMaterials = materials.size();
    header.numSceneNodes = sceneNodes.size();
    header.numVertices = vertexData.size();
    header.numTextures = textures.size();
    binFile.write(reinterpret_cast<const char*>(&header), sizeof(BinCacheFileHeader));

    // Write materials in key order for deterministic cache output.
    std::map<std::string, const Material*> orderedMaterials;
    for (const auto& [name, mat] : materials)
    {
        orderedMaterials.emplace(name, &mat);
    }

    // Write materials (field-by-field, no raw struct dump)
    for (const auto& [name, matPtr] : orderedMaterials)
    {
        const Material& mat = *matPtr;
        binFile.write(mat.name, sizeof(mat.name));
        binFile.write(reinterpret_cast<const char*>(mat.ambient), sizeof(mat.ambient));
        binFile.write(reinterpret_cast<const char*>(mat.diffuse), sizeof(mat.diffuse));
        binFile.write(reinterpret_cast<const char*>(mat.specular), sizeof(mat.specular));
        binFile.write(reinterpret_cast<const char*>(mat.transmittance), sizeof(mat.transmittance));
        binFile.write(reinterpret_cast<const char*>(mat.emission), sizeof(mat.emission));
        binFile.write(reinterpret_cast<const char*>(&mat.shininess), sizeof(mat.shininess));
        binFile.write(reinterpret_cast<const char*>(&mat.ior), sizeof(mat.ior));
        binFile.write(reinterpret_cast<const char*>(&mat.dissolve), sizeof(mat.dissolve));
        binFile.write(reinterpret_cast<const char*>(&mat.illum), sizeof(mat.illum));
        binFile.write(mat.diffuseTexName, sizeof(mat.diffuseTexName));
        binFile.write(mat.normalTexName, sizeof(mat.normalTexName));
        binFile.write(mat.specularTexName, sizeof(mat.specularTexName));
    }

    // Write scene nodes (field-by-field with vertex data)
    for (const auto& node : sceneNodes)
    {
        binFile.write(node.name, sizeof(node.name));
        binFile.write(node.material, sizeof(node.material));
        const size_t serializedVertexDataSize = (node.vertexData && node.vertexDataSize > 0) ? node.vertexDataSize : 0;
        binFile.write(reinterpret_cast<const char*>(&serializedVertexDataSize), sizeof(serializedVertexDataSize));
        
        if (serializedVertexDataSize > 0)
        {
            binFile.write(reinterpret_cast<const char*>(node.vertexData.get()), 
                         sizeof(Vertex) * serializedVertexDataSize);
        }
        
        binFile.write(reinterpret_cast<const char*>(&node.modelViewMatrix), sizeof(node.modelViewMatrix));
        binFile.write(reinterpret_cast<const char*>(&node.startPosition), sizeof(node.startPosition));
        binFile.write(reinterpret_cast<const char*>(&node.endPosition), sizeof(node.endPosition));
        binFile.write(reinterpret_cast<const char*>(&node.primitiveMode), sizeof(node.primitiveMode));
        binFile.write(reinterpret_cast<const char*>(&node.ambientTextureId), sizeof(node.ambientTextureId));
        binFile.write(reinterpret_cast<const char*>(&node.diffuseTextureId), sizeof(node.diffuseTextureId));
        binFile.write(reinterpret_cast<const char*>(&node.normalTextureId), sizeof(node.normalTextureId));
        binFile.write(reinterpret_cast<const char*>(&node.specularTextureId), sizeof(node.specularTextureId));
        binFile.write(reinterpret_cast<const char*>(&node.boundingSphere), sizeof(node.boundingSphere));
        binFile.write(reinterpret_cast<const char*>(&node.lx), sizeof(node.lx));
        binFile.write(reinterpret_cast<const char*>(&node.ly), sizeof(node.ly));
        binFile.write(reinterpret_cast<const char*>(&node.lz), sizeof(node.lz));
    }

    // Write vertex data
    for (const auto& v : vertexData)
    {
        binFile.write(reinterpret_cast<const char*>(&v), sizeof(Vertex));
    }

    // Write textures with proper data handling
    for (const auto& [name, texture] : textures)
    {
        std::string nameStr = name;
        if (nameStr.size() >= MAX_MATERIAL_NAME_STRING_LENGTH)
            nameStr.resize(MAX_MATERIAL_NAME_STRING_LENGTH - 1);
        
        binFile.write(nameStr.c_str(), MAX_MATERIAL_NAME_STRING_LENGTH);
        binFile.write(reinterpret_cast<const char*>(&texture->bpp), sizeof(unsigned));
        binFile.write(reinterpret_cast<const char*>(&texture->mode), sizeof(int));
        binFile.write(reinterpret_cast<const char*>(&texture->width), sizeof(unsigned));
        binFile.write(reinterpret_cast<const char*>(&texture->height), sizeof(unsigned));
        
        if (texture->data && texture->isValid())
        {
            size_t dataSize = static_cast<size_t>(texture->width) * texture->height * texture->bpp;
            binFile.write(reinterpret_cast<const char*>(texture->data.get()), static_cast<std::streamsize>(dataSize));
        }
    }

    binFile.close();
    
    if (isVerboseEnabled())
        std::cout << "saved cache to " << filename << std::endl;

    return 0;
}

bool Renderer::checkScene() const
{
    if (sceneNodes.empty())
    {
        std::cout << "building empty scene" << std::endl;
        return false;
    }
    return true;
}

bool Renderer::buildScene(Camera& camera)
{
    (void)camera;
    
    // Populate vertexData and indices from sceneNodes while preserving each node's
    // global draw range in the flattened buffer. This is required because the draw
    // path reads node.startPosition/node.endPosition as offsets into the merged VBO.
    for (auto& node : sceneNodes)
    {
        const GLuint vertexOffset = static_cast<GLuint>(vertexData.size());
        node.setDrawRange(vertexOffset);
        for (size_t j = 0; j < node.vertexDataSize; ++j)
        {
            vertexData.push_back(node.vertexData[j]);
            indices.push_back(static_cast<GLuint>(indices.size()));
        }
    }

    // Calculate bounding sphere radius and local origin (single-pass optimization)
    for (auto& node : sceneNodes)
    {
        if (node.vertexDataSize == 0)
        {
            node.lx = node.ly = node.lz = 0.0f;
            node.boundingSphere = 0.1f;
            continue;
        }

        Math::Sphere sphere = Math::calculateBoundingSphere(node.vertexData.get(), node.vertexDataSize);
        node.lx = sphere.center.x;
        node.ly = sphere.center.y;
        node.lz = sphere.center.z;
        node.boundingSphere = sphere.radius;
        
        if (node.boundingSphere == 0.0f)
            node.boundingSphere = 0.1f;
    }

    // Free per-node vertex data (now in global vertexData)
    for (auto& node : sceneNodes)
    {
        node.vertexData.reset();
    }

    ++sceneRevision;
    shadowDirty = true;

    return checkScene();
}

bool Renderer::buildScene(Camera& camera, std::string_view cacheFilename)
{
    (void)camera;

    const std::string filename(cacheFilename);
    std::ifstream binFile(filename, std::ios::in | std::ios::binary);
    if (!binFile.is_open())
    {
        return false;
    }

    const auto clearSceneData = [this]() {
        materials.clear();
        sceneNodes.clear();
        vertexData.clear();
        indices.clear();
        textures.clear();
    };

    const auto failLoad = [&](const char* message) {
        std::cerr << message << std::endl;
        clearSceneData();
        return false;
    };

    const auto readExact = [&](void* dst, size_t bytes) -> bool {
        if (bytes == 0) return true;
        binFile.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(bytes));
        return binFile.good();
    };

    const auto boundedStringLength = [](const char* s, size_t maxLen) {
        size_t len = 0;
        while (len < maxLen && s[len] != '\0')
        {
            ++len;
        }
        return len;
    };

    auto remainingBytes = [&]() -> uint64_t {
        const std::streampos pos = binFile.tellg();
        if (pos < 0) return 0;
        binFile.seekg(0, std::ios::end);
        const std::streampos endPos = binFile.tellg();
        binFile.seekg(pos, std::ios::beg);
        if (endPos < pos) return 0;
        return static_cast<uint64_t>(endPos - pos);
    };

    try
    {
        clearSceneData();

        BinCacheFileHeader header{};
        if (!readExact(&header, sizeof(BinCacheFileHeader)))
        {
            return failLoad("Unable to read cache header");
        }

        if (header.magic != 0x5741564F || header.version != BINARY_CACHE_VERSION)
        {
            std::cerr << "Incompatible cache format: expected v" << BINARY_CACHE_VERSION
                      << ", got v" << header.version << std::endl;
            return false;
        }

        constexpr size_t kMaxMaterialCount = 100000;
        constexpr size_t kMaxSceneNodeCount = 100000;
        constexpr size_t kMaxVertexCount = 50000000;
        constexpr size_t kMaxTextureCount = 100000;
        constexpr size_t kMaxNodeVertexCount = 50000000;

        if (header.numMaterials > kMaxMaterialCount ||
            header.numSceneNodes > kMaxSceneNodeCount ||
            header.numVertices > kMaxVertexCount ||
            header.numTextures > kMaxTextureCount)
        {
            return failLoad("Cache header contains unreasonable counts");
        }

        for (size_t i = 0; i < header.numMaterials; ++i)
        {
            Material m{};
            if (!readExact(m.name, sizeof(m.name)) ||
                !readExact(m.ambient, sizeof(m.ambient)) ||
                !readExact(m.diffuse, sizeof(m.diffuse)) ||
                !readExact(m.specular, sizeof(m.specular)) ||
                !readExact(m.transmittance, sizeof(m.transmittance)) ||
                !readExact(m.emission, sizeof(m.emission)) ||
                !readExact(&m.shininess, sizeof(m.shininess)) ||
                !readExact(&m.ior, sizeof(m.ior)) ||
                !readExact(&m.dissolve, sizeof(m.dissolve)) ||
                !readExact(&m.illum, sizeof(m.illum)) ||
                !readExact(m.diffuseTexName, sizeof(m.diffuseTexName)) ||
                !readExact(m.normalTexName, sizeof(m.normalTexName)) ||
                !readExact(m.specularTexName, sizeof(m.specularTexName)))
            {
                return failLoad("Cache file ended while reading materials");
            }

            m.name[sizeof(m.name) - 1] = '\0';
            if (m.name[0] != '\0')
            {
                const size_t nameLen = boundedStringLength(m.name, sizeof(m.name));
                materials.emplace(std::string(m.name, nameLen), m);
            }
        }

        for (size_t i = 0; i < header.numSceneNodes; ++i)
        {
            SceneNode node{};
            if (!readExact(node.name, sizeof(node.name)) ||
                !readExact(node.material, sizeof(node.material)) ||
                !readExact(&node.vertexDataSize, sizeof(node.vertexDataSize)))
            {
                return failLoad("Cache file ended while reading scene node header");
            }

            node.name[sizeof(node.name) - 1] = '\0';
            node.material[sizeof(node.material) - 1] = '\0';

            if (node.vertexDataSize > kMaxNodeVertexCount)
            {
                return failLoad("Cache node vertex count is too large");
            }

            const uint64_t vertexBytes = static_cast<uint64_t>(node.vertexDataSize) * static_cast<uint64_t>(sizeof(Vertex));
            if (vertexBytes > remainingBytes())
            {
                return failLoad("Cache node vertex data is truncated");
            }

            if (node.vertexDataSize > 0)
            {
                node.vertexData = std::make_unique<Vertex[]>(node.vertexDataSize);
                if (!readExact(node.vertexData.get(), static_cast<size_t>(vertexBytes)))
                {
                    return failLoad("Cache file ended while reading scene node vertices");
                }
            }

            if (!readExact(&node.modelViewMatrix, sizeof(node.modelViewMatrix)) ||
                !readExact(&node.startPosition, sizeof(node.startPosition)) ||
                !readExact(&node.endPosition, sizeof(node.endPosition)) ||
                !readExact(&node.primitiveMode, sizeof(node.primitiveMode)) ||
                !readExact(&node.ambientTextureId, sizeof(node.ambientTextureId)) ||
                !readExact(&node.diffuseTextureId, sizeof(node.diffuseTextureId)) ||
                !readExact(&node.normalTextureId, sizeof(node.normalTextureId)) ||
                !readExact(&node.specularTextureId, sizeof(node.specularTextureId)) ||
                !readExact(&node.boundingSphere, sizeof(node.boundingSphere)) ||
                !readExact(&node.lx, sizeof(node.lx)) ||
                !readExact(&node.ly, sizeof(node.ly)) ||
                !readExact(&node.lz, sizeof(node.lz)))
            {
                return failLoad("Cache file ended while reading scene node payload");
            }

            node.diffuseTextureId = 0;
            sceneNodes.push_back(std::move(node));
        }

        vertexData.reserve(header.numVertices);
        indices.reserve(header.numVertices);
        for (size_t i = 0; i < header.numVertices; ++i)
        {
            Vertex v{};
            if (!readExact(&v, sizeof(Vertex)))
            {
                return failLoad("Cache file ended while reading vertex data");
            }
            vertexData.push_back(v);
            indices.push_back(static_cast<GLuint>(indices.size()));
        }

        size_t texturesLoaded = 0;
        while (texturesLoaded < header.numTextures)
        {
            char nameBuf[MAX_MATERIAL_NAME_STRING_LENGTH]{};
            unsigned texWidth = 0;
            unsigned texHeight = 0;
            unsigned texBpp = 0;
            int texMode = 0;
            if (!readExact(nameBuf, MAX_MATERIAL_NAME_STRING_LENGTH) ||
                !readExact(&texBpp, sizeof(unsigned)) ||
                !readExact(&texMode, sizeof(int)) ||
                !readExact(&texWidth, sizeof(unsigned)) ||
                !readExact(&texHeight, sizeof(unsigned)))
            {
                return failLoad("Cache file ended while reading texture header");
            }

            if (texWidth == 0 || texHeight == 0 || texBpp == 0 || texBpp > 16)
            {
                return failLoad("Cache texture header is invalid");
            }

            const uint64_t imageSize64 = static_cast<uint64_t>(texWidth) *
                                         static_cast<uint64_t>(texHeight) *
                                         static_cast<uint64_t>(texBpp);
            if (imageSize64 == 0 || imageSize64 > remainingBytes() || imageSize64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
            {
                return failLoad("Cache texture data is truncated or invalid");
            }

            const size_t imageSize = static_cast<size_t>(imageSize64);
            auto texture = std::make_shared<Texture>();
            texture->width = texWidth;
            texture->height = texHeight;
            texture->bpp = texBpp;
            texture->mode = texMode;
            texture->data = std::shared_ptr<unsigned char[]>(new unsigned char[imageSize], [](unsigned char* p) { delete[] p; });
            if (!readExact(texture->data.get(), imageSize))
            {
                return failLoad("Cache file ended while reading texture data");
            }

            nameBuf[sizeof(nameBuf) - 1] = '\0';
            const size_t nameLen = boundedStringLength(nameBuf, sizeof(nameBuf));
            const std::string nameStr(nameBuf, nameLen);
            textures[nameStr] = texture;
            ++texturesLoaded;
        }
    }
    catch (const std::bad_alloc&)
    {
        return failLoad("Cache load failed: out of memory");
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Cache load exception: " << ex.what() << std::endl;
        clearSceneData();
        return false;
    }

    ++sceneRevision;
    shadowDirty = true;
    return checkScene();
}

void Renderer::bufferToGpu(Camera& camera, bool loadCachedScene)
{
    backend->bufferToGpu(camera, cacheFileName, loadCachedScene);

    if (!loadCachedScene && configLoader->getBool("renderer.createBinObj"))
    {
        if (binCacheWriterThread != nullptr)
        {
            int status = 0;
            SDL_WaitThread(static_cast<SDL_Thread*>(binCacheWriterThread), &status);
            binCacheWriterThread = nullptr;
        }
        binCacheWriterThread = SDL_CreateThread(createBinCacheThread, "BinCacheWriterThread", this);
    }

    rebuildScenegraph();

    if (occlusionCullingEnabled)
    {
        occlusionQueries.resize(sceneNodes.size(), 0);
        glGenQueries(static_cast<GLsizei>(occlusionQueries.size()), occlusionQueries.data());
        occlusionVisible.assign(sceneNodes.size(), 1);
        occlusionSkipCounters.assign(sceneNodes.size(), 0);
    }
}

int Renderer::buildCullNode(std::vector<int>& sortedIndices, int start, int end)
{
    CullNode node{};
    node.leftChild = static_cast<std::size_t>(-1);
    node.rightChild = static_cast<std::size_t>(-1);
    node.firstLeaf = static_cast<std::size_t>(-1);
    node.leafCount = end - start;

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    for (int i = start; i < end; ++i)
    {
        const SceneNode& sn = sceneNodes[sortedIndices[i]];
        sumX += sn.lx; sumY += sn.ly; sumZ += sn.lz;
        if (sn.lx < minX) minX = sn.lx;
        if (sn.ly < minY) minY = sn.ly;
        if (sn.lz < minZ) minZ = sn.lz;
        if (sn.lx > maxX) maxX = sn.lx;
        if (sn.ly > maxY) maxY = sn.ly;
        if (sn.lz > maxZ) maxZ = sn.lz;
    }

    const float invCount = 1.0f / static_cast<float>(end - start);
    node.cx = sumX * invCount;
    node.cy = sumY * invCount;
    node.cz = sumZ * invCount;
    node.radius = 0.0f;

    for (int i = start; i < end; ++i)
    {
        const SceneNode& sn = sceneNodes[sortedIndices[i]];
        const float dx = sn.lx - node.cx;
        const float dy = sn.ly - node.cy;
        const float dz = sn.lz - node.cz;
        const float dist = static_cast<float>(std::sqrt(static_cast<double>(dx*dx + dy*dy + dz*dz))) + sn.boundingSphere;
        if (dist > node.radius) node.radius = dist;
    }

    int nodeIndex = static_cast<int>(cullNodes.size());
    cullNodes.push_back(node);

    if ((end - start) <= cullLeafSize)
    {
        cullNodes[nodeIndex].firstLeaf = static_cast<std::size_t>(cullLeafNodeIndices.size());
        cullNodes[nodeIndex].leafCount = end - start;
        for (int i = start; i < end; ++i)
            cullLeafNodeIndices.push_back(sortedIndices[i]);
        return nodeIndex;
    }

    float extentX = maxX - minX, extentY = maxY - minY, extentZ = maxZ - minZ;
    int splitAxis = 0;
    if (extentY > extentX && extentY > extentZ) splitAxis = 1;
    else if (extentZ > extentX && extentZ > extentY) splitAxis = 2;

    std::sort(sortedIndices.begin() + start, sortedIndices.begin() + end,
        [splitAxis, this](int a, int b) {
            const auto& getVal = [this](const SceneNode& n, int axis) {
                if (axis == 0) return n.lx;
                if (axis == 1) return n.ly;
                return n.lz;
            };
            return getVal(sceneNodes[a], splitAxis) < getVal(sceneNodes[b], splitAxis);
        });

    const int mid = start + ((end - start) / 2);
    cullNodes[nodeIndex].leftChild = static_cast<std::size_t>(buildCullNode(sortedIndices, start, mid));
    cullNodes[nodeIndex].rightChild = static_cast<std::size_t>(buildCullNode(sortedIndices, mid, end));
    return nodeIndex;
}

void Renderer::rebuildScenegraph()
{
    cullNodes.clear();
    cullLeafNodeIndices.clear();
    if (sceneNodes.empty() || !hierarchicalCullingEnabled) return;

    std::vector<int> nodeIndices;
    nodeIndices.reserve(static_cast<size_t>(sceneNodes.size()));
    for (size_t i = 0; i < sceneNodes.size(); ++i)
        nodeIndices.push_back(static_cast<int>(i));
    
    buildCullNode(nodeIndices, 0, static_cast<int>(nodeIndices.size()));
}

void Renderer::collectVisibleNodes(std::vector<int>& outVisible)
{
    outVisible.clear();
    if (sceneNodes.empty()) return;

    if (cullNodes.empty())
    {
        outVisible.reserve(sceneNodes.size());
        for (size_t i = 0; i < sceneNodes.size(); ++i)
        {
            const auto& sn = sceneNodes[i];
            if (frustum.spherePartiallyInFrustum(sn.lx, sn.ly, sn.lz, sn.boundingSphere) > 0)
                outVisible.push_back(static_cast<int>(i));
        }
        return;
    }

    std::vector<int> stack;
    stack.reserve(cullNodes.size());
    stack.push_back(0);
    
    while (!stack.empty())
    {
        int nodeIndex = stack.back();
        stack.pop_back();
        const auto& node = cullNodes[static_cast<size_t>(nodeIndex)];
        
        if (frustum.spherePartiallyInFrustum(node.cx, node.cy, node.cz, node.radius) == 0)
            continue;

        if (node.leftChild == static_cast<std::size_t>(-1) && node.rightChild == static_cast<std::size_t>(-1))
        {
            for (int i = 0; i < node.leafCount; ++i)
            {
                const int sceneIdx = cullLeafNodeIndices[node.firstLeaf + static_cast<size_t>(i)];
                const auto& sn = sceneNodes[static_cast<size_t>(sceneIdx)];
                if (frustum.spherePartiallyInFrustum(sn.lx, sn.ly, sn.lz, sn.boundingSphere) > 0)
                    outVisible.push_back(sceneIdx);
            }
        }
        else
        {
            if (node.leftChild != static_cast<std::size_t>(-1)) stack.push_back(static_cast<int>(node.leftChild));
            if (node.rightChild != static_cast<std::size_t>(-1)) stack.push_back(static_cast<int>(node.rightChild));
        }
    }
}

void Renderer::updateOcclusionQueryResults()
{
    if (!occlusionCullingEnabled || occlusionQueries.empty()) return;

    for (size_t i = 0; i < occlusionQueries.size(); ++i)
    {
        GLuint query = occlusionQueries[i];
        if (query == 0) continue;
        
        GLuint available = 0;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
        if (available)
        {
            GLuint samples = 0;
            glGetQueryObjectuiv(query, GL_QUERY_RESULT, &samples);
            occlusionVisible[i] = (samples >= static_cast<GLuint>(occlusionMinSamples)) ? 1 : 0;
        }
    }
}

void Renderer::enableShadows() { shadowsEnabled = true; }
void Renderer::disableShadows() { shadowsEnabled = false; }

GLuint Renderer::createShadowMap(Camera& camera)
{
    if (shadowMap == 0)
    {
        shadowMap = backend->createShadowMap(camera);
        backend->getShadowMapSize(shadowWidth, shadowHeight);
    }
    return shadowMap;
}

void Renderer::render(Camera& camera, const FrameContext& frameContext)
{
    (void)frameContext;

    if (sceneNodes.empty())
    {
        std::cout << "skipping render() on empty scene" << std::endl;
        return;
    }

    Uint64 frameStartCounter = SDL_GetPerformanceCounter();
    double shadowPassMs = 0.0;
    int drawCalls = 0;
    int visibleNodes = 0;
    int occlusionCulledNodes = 0;

    // Fix: Use consistent light position (was previously camera.position + glm::vec3(0.0f, 100.0f, 0.0f) in bufferToGpu)
    const glm::vec3 lightPos = camera.position + Math::getLightPositionOffset();
    glm::mat4 lightProjection, lightView, lightSpaceMatrix;
    
    backend->fitDirectionalShadowMatrix(camera, lightPos, shadowWidth, shadowHeight,
                                               lightView, lightProjection, lightSpaceMatrix);

    if (shadowsEnabled)
    {
        const bool lightMoved = !shadowInitialized || glm::distance(lastShadowLightPos, lightPos) > 0.001f;
        const bool sceneChanged = (sceneRevision != shadowSceneRevision);
        shadowDirty = shadowDirty || lightMoved || sceneChanged;

        if (shadowDirty)
        {
            Uint64 shadowStart = SDL_GetPerformanceCounter();
            shadowMap = createShadowMap(camera);
            const double perfFreq = static_cast<double>(SDL_GetPerformanceFrequency());
            shadowPassMs = (static_cast<double>(SDL_GetPerformanceCounter() - shadowStart) * 1000.0) / perfFreq;

            lastShadowLightPos = lightPos;
            shadowSceneRevision = sceneRevision;
            shadowInitialized = true;
            shadowDirty = false;
        }
    }

    // Fix: Pass the computed light space matrix to the backend for per-frame updates
    if (backend)
    {
        backend->updateLightUniforms(camera, lightSpaceMatrix, lightPos);
    }

    backend->beginFrame(frameContext);

    if (frustumCullingEnabled)
    {
        frustum.extractFrustum(camera.projectionMatrix * camera.modelViewMatrix);
    }
    updateOcclusionQueryResults();

    visibleNodeIdsScratch.clear();
    if (frustumCullingEnabled)
    {
        collectVisibleNodes(visibleNodeIdsScratch);
    }
    else
    {
        visibleNodeIdsScratch.reserve(sceneNodes.size());
        for (size_t i = 0; i < sceneNodes.size(); ++i)
        {
            visibleNodeIdsScratch.push_back(static_cast<int>(i));
        }
    }

    // Fallback: if culling rejects everything, render all nodes to avoid a black frame.
    if (visibleNodeIdsScratch.empty() && !sceneNodes.empty())
    {
        visibleNodeIdsScratch.reserve(sceneNodes.size());
        for (size_t i = 0; i < sceneNodes.size(); ++i)
        {
            visibleNodeIdsScratch.push_back(static_cast<int>(i));
        }
    }

    const int frustumCulledNodes = static_cast<int>(sceneNodes.size()) - static_cast<int>(visibleNodeIdsScratch.size());

    visibleNodesSortedScratch.clear();
    visibleNodesSortedScratch.reserve(visibleNodeIdsScratch.size());
    
    for (int sceneIdx : visibleNodeIdsScratch)
    {
        visibleNodesSortedScratch.push_back({sceneNodes[static_cast<size_t>(sceneIdx)].diffuseTextureId,
                                             sceneNodes[static_cast<size_t>(sceneIdx)].startPosition,
                                             sceneIdx});
    }

    if (occlusionCullingEnabled && !occlusionQueries.empty())
    {
        occlusionFilteredScratch.clear();
        occlusionFilteredScratch.reserve(visibleNodesSortedScratch.size());
        for (auto& key : visibleNodesSortedScratch)
        {
            const int sceneIdx = key.nodeIndex;
            if (!occlusionVisible[static_cast<size_t>(sceneIdx)])
            {
                occlusionSkipCounters[static_cast<size_t>(sceneIdx)]++;
                if (occlusionSkipCounters[static_cast<size_t>(sceneIdx)] < occlusionRetestFrames)
                {
                    ++occlusionCulledNodes;
                    continue;
                }
            }
            occlusionSkipCounters[static_cast<size_t>(sceneIdx)] = 0;
            occlusionFilteredScratch.push_back(key);
        }
        visibleNodesSortedScratch.swap(occlusionFilteredScratch);
    }

    // Texture bucketing reduces wide key comparisons versus global sort.
    textureBucketOrderScratch.clear();
    for (auto& [textureId, bucket] : textureBucketsScratch)
    {
        bucket.clear();
    }

    for (const auto& key : visibleNodesSortedScratch)
    {
        auto& bucket = textureBucketsScratch[key.textureId];
        if (bucket.empty())
        {
            textureBucketOrderScratch.push_back(key.textureId);
        }
        bucket.push_back(key);
    }

    std::sort(textureBucketOrderScratch.begin(), textureBucketOrderScratch.end());

    visibleNodesSortedScratch.clear();
    for (GLuint textureId : textureBucketOrderScratch)
    {
        auto& bucket = textureBucketsScratch[textureId];
        std::sort(bucket.begin(), bucket.end(),
            [](const SortKey& a, const SortKey& b) {
                return a.startPosition < b.startPosition;
            });
        visibleNodesSortedScratch.insert(visibleNodesSortedScratch.end(), bucket.begin(), bucket.end());
    }

    renderCommandsScratch.clear();
    renderCommandsScratch.reserve(visibleNodesSortedScratch.size());
    for (const auto& key : visibleNodesSortedScratch)
    {
        RenderCommand cmd;
        cmd.type = RenderCommand::CommandType::DRAW_ELEMENTS;
        cmd.node = &sceneNodes[static_cast<size_t>(key.nodeIndex)];
        renderCommandsScratch.push_back(cmd);
    }

    backend->submit(renderCommandsScratch);
    checkForGLError();
    backend->endFrame();

    drawCalls = static_cast<int>(renderCommandsScratch.size());
    visibleNodes = static_cast<int>(visibleNodesSortedScratch.size());

    const Uint64 frameEndCounter = SDL_GetPerformanceCounter();
    const double perfFreq = static_cast<double>(SDL_GetPerformanceFrequency());
    
    perfStats.cpuFrameMs = (static_cast<double>(frameEndCounter - frameStartCounter) * 1000.0) / perfFreq;
    perfStats.shadowPassMs = shadowPassMs;
    perfStats.drawCalls = drawCalls;
    perfStats.visibleNodes = visibleNodes;
    perfStats.totalNodes = static_cast<int>(sceneNodes.size());
    perfStats.frustumCulledNodes = frustumCulledNodes;
    perfStats.occlusionCulledNodes = occlusionCulledNodes;
    perfStats.camera = &camera;
    
    if (lastPerfCounter != 0)
    {
        const double frameSeconds = static_cast<double>(frameEndCounter - lastPerfCounter) / perfFreq;
        if (frameSeconds > 0.0) perfStats.fps = 1.0 / frameSeconds;
    }
    lastPerfCounter = frameEndCounter;
}

bool Renderer::isVerboseEnabled() const
{
    return configLoader ? configLoader->getBool("renderer.verbose") : false;
}
