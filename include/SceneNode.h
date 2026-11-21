#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

#include "Common.h"
#include "Material.h"
#include <array>
#include <algorithm>
#include <cstring>
#include <memory>
#include <string_view>

// ============================================================================
// SceneNode (renderable mesh instance with texture bindings)
// ============================================================================
struct SceneNode {
    char name[MAX_NODE_NAME_STRING_LENGTH]{};
    char material[MAX_MATERIAL_NAME_STRING_LENGTH]{};
    std::unique_ptr<Vertex[]> vertexData;
    size_t vertexDataSize = 0;
    glm::mat4 modelViewMatrix{1.0f};
    GLuint startPosition = 0;
    GLuint endPosition = 0;
    GLenum primitiveMode = GL_TRIANGLES;

    GLuint ambientTextureId = 0;
    GLuint diffuseTextureId = 0;
    GLuint normalTextureId = 0;
    GLuint specularTextureId = 0;

    GLfloat boundingSphere = 0.0f;
    GLfloat lx = 0.0f, ly = 0.0f, lz = 0.0f;
    
    // Default constructor
    SceneNode() = default;
    
    // Move constructor - efficient transfer of ownership
    SceneNode(SceneNode&& other) noexcept 
        : vertexData(std::move(other.vertexData)), 
          vertexDataSize(other.vertexDataSize),
          modelViewMatrix(other.modelViewMatrix), 
          startPosition(other.startPosition),
          endPosition(other.endPosition), 
          primitiveMode(other.primitiveMode),
          ambientTextureId(other.ambientTextureId), 
          diffuseTextureId(other.diffuseTextureId),
          normalTextureId(other.normalTextureId), 
          specularTextureId(other.specularTextureId),
          boundingSphere(other.boundingSphere), 
          lx(other.lx), ly(other.ly), lz(other.lz) 
    {
        std::memcpy(name, other.name, sizeof(name));
        std::memcpy(material, other.material, sizeof(material));
        other.vertexDataSize = 0;
    }
    
    // Delete copy operations (SceneNode owns vertexData)
    SceneNode(const SceneNode&) = delete;
    SceneNode& operator=(const SceneNode&) = delete;
    SceneNode& operator=(SceneNode&&) = delete;
    
    // Helper to set name safely
    void setName(std::string_view n) {
        std::memset(name, 0, sizeof(name));
        if (!n.empty()) {
            std::memcpy(name, n.data(), (std::min)(n.size(), sizeof(name) - 1));
        }
    }
    
    // Helper to set material safely
    void setMaterial(std::string_view m) {
        std::memset(material, 0, sizeof(material));
        if (!m.empty()) {
            std::memcpy(material, m.data(), (std::min)(m.size(), sizeof(material) - 1));
        }
    }

    /** Return the number of vertices in this node. */
    [[nodiscard]] GLsizei vertexCount() const {
        return static_cast<GLsizei>(endPosition - startPosition);
    }

    void setDrawRange(GLuint baseIndex) {
        startPosition = baseIndex;
        endPosition = baseIndex + static_cast<GLuint>(vertexDataSize);
    }

    /** Check if this node has valid geometry. */
    [[nodiscard]] bool hasGeometry() const {
        return vertexData != nullptr && vertexDataSize > 0;
    }
};

#endif // _SCENE_NODE_H_
