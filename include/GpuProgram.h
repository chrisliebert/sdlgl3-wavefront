#ifndef _GPU_PROGRAM_H_
#define _GPU_PROGRAM_H_

#include "Common.h"
#include "Shader.h"
#include <memory>
#include <map>
#include <string>
#include <string_view>

class Uniform {
public:
    enum class Type { Mat4, Vec3, Int };
    virtual Type getType() const = 0;
protected:
    GLuint location;
public:
    virtual void load() = 0;
    [[nodiscard]] GLuint getLocation();
    void setLocation(GLuint);
    virtual ~Uniform() = default;
};

class UniformMat4 : public Uniform {
private:
    glm::mat4 matrix;
public:
    explicit UniformMat4(const glm::mat4& mat);
    Type getType() const override { return Type::Mat4; }
    void load() override;
    void set(const glm::mat4& mat);
};

class UniformVec3 : public Uniform {
private:
    glm::vec3 vector;
public:
    explicit UniformVec3(const glm::vec3& vec);
    Type getType() const override { return Type::Vec3; }
    void load() override;
    void set(const glm::vec3& vec);
};

class UniformInt : public Uniform {
private:
    GLint i;
public:
    explicit UniformInt(GLint val);
    Type getType() const override { return Type::Int; }
    void load() override;
    void set(GLint val);
};

// Using unique_ptr for automatic memory management
using UniformPtr = std::unique_ptr<Uniform>;

class UniformLoader {
private:
    GLuint programId;
    std::map<std::string, UniformPtr> uniforms;
    std::vector<Uniform*> uniformVector;
public:
    explicit UniformLoader(GLuint programId);
    ~UniformLoader();
    
    // Non-copyable
    UniformLoader(const UniformLoader&) = delete;
    UniformLoader& operator=(const UniformLoader&) = delete;
    
    void addUniform(std::string_view name, UniformPtr uniform);
    Uniform* get(std::string_view name) const;
    
    // Fast index-based lookup
    size_t cacheIndex(std::string_view name);
    Uniform* getByIndex(size_t idx) const;
    
    void load() const;
};

class GpuProgram {
public:
    GpuProgram();
    ~GpuProgram();
    
    [[nodiscard]] GLuint getId() const;
    void attachShader(Shader& _shader);
    void use() const;
    
    // Non-copyable, movable
    GpuProgram(const GpuProgram&) = delete;
    GpuProgram& operator=(const GpuProgram&) = delete;
    GpuProgram(GpuProgram&&) = default;
    GpuProgram& operator=(GpuProgram&&) = default;
    
    [[nodiscard]] UniformLoader* getUniformLoader() const { return uniformLoader.get(); }

private:
    GLuint id;
    std::unique_ptr<UniformLoader> uniformLoader;
};

#endif // _GPU_PROGRAM_H_
