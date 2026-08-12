#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include "Common.h"
#include <array>
#include <cstring>
#include <memory>
#include <string_view>

// Binary cache format version - increment when serialization changes
inline constexpr uint32_t BINARY_CACHE_VERSION = 6;

// ============================================================================
// Material (PBR-extended Phong material for .mtl loading)
// ============================================================================
struct Material {
    char name[MAX_MATERIAL_NAME_STRING_LENGTH]{};
    float ambient[3]{0.0f, 0.0f, 0.0f};
    float diffuse[3]{0.0f, 0.0f, 0.0f};
    float specular[3]{0.0f, 0.0f, 0.0f};
    float transmittance[3]{0.0f, 0.0f, 0.0f};
    float emission[3]{0.0f, 0.0f, 0.0f};
    float shininess = 0.0f;
    float ior = 1.0f;
    float dissolve = 1.0f;
    int illum = 0;

    char ambientTexName[MAX_MATERIAL_NAME_STRING_LENGTH]{};
    char diffuseTexName[MAX_MATERIAL_NAME_STRING_LENGTH]{};
    char specularTexName[MAX_MATERIAL_NAME_STRING_LENGTH]{};
    char normalTexName[MAX_MATERIAL_NAME_STRING_LENGTH]{};

    void setName(std::string_view n) {
        std::memset(name, 0, sizeof(name));
        if (!n.empty())
            std::memcpy(name, n.data(), (n.size() < sizeof(name) - 1) ? n.size() : sizeof(name) - 1);
    }

    void setDiffuseTexName(std::string_view n) {
        std::memset(diffuseTexName, 0, sizeof(diffuseTexName));
        if (!n.empty())
            std::memcpy(diffuseTexName, n.data(), (n.size() < sizeof(diffuseTexName) - 1) ? n.size() : sizeof(diffuseTexName) - 1);
    }

    void setNormalTexName(std::string_view n) {
        std::memset(normalTexName, 0, sizeof(normalTexName));
        if (!n.empty())
            std::memcpy(normalTexName, n.data(), (n.size() < sizeof(normalTexName) - 1) ? n.size() : sizeof(normalTexName) - 1);
    }

    void setSpecularTexName(std::string_view n) {
        std::memset(specularTexName, 0, sizeof(specularTexName));
        if (!n.empty())
            std::memcpy(specularTexName, n.data(), (n.size() < sizeof(specularTexName) - 1) ? n.size() : sizeof(specularTexName) - 1);
    }

    [[nodiscard]] bool hasDiffuseTexture() const {
        return diffuseTexName[0] != '\0';
    }
};

// ============================================================================
// Texture (texture data with shared ownership semantics)
// ============================================================================
struct Texture {
    unsigned width = 0;
    unsigned height = 0;
    unsigned bpp = 0;
    int mode = GL_RGB;
    
    // Pixel data with ownership via shared_ptr and custom deleter
    std::shared_ptr<unsigned char[]> data;
    
    // Default constructor - no data ownership
    Texture() = default;
    
    // Constructor with data ownership
    explicit Texture(unsigned char* rawData, size_t /*dataSize*/)
        : data(rawData, [](unsigned char* p) { delete[] p; })
    {}
    
    // Copy constructor - shares ownership
    Texture(const Texture& other) = default;
    
    // Move constructor
    Texture(Texture&& other) noexcept = default;
    
    // Assignment operators
    Texture& operator=(const Texture& other) = default;
    Texture& operator=(Texture&& other) noexcept = default;
    
    // Destructor - handled by shared_ptr
    ~Texture() = default;
    
    // Check if texture has valid data
    [[nodiscard]] bool isValid() const { return data != nullptr && width > 0 && height > 0; }
    
    // Get data size in bytes
    [[nodiscard]] size_t getDataSize() const { 
        return data ? (width * height * bpp) : 0; 
    }
};

#endif // _MATERIAL_H_
