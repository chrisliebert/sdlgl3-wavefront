#ifndef _GPU_PROGRAM_H_
#define _GPU_PROGRAM_H_

#include "Common.h"
#include "Shader.h"
#include <memory>
#include <map>
#include <string>
#include <string_view>

class Uniform {
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
    void load() override;
    void set(const glm::mat4& mat);
};

class UniformVec3 : public Uniform {
private:
    glm::vec3 vector;
public:
    explicit UniformVec3(const glm::vec3& vec);
    void load() override;
    void set(const glm::vec3& vec);
};

class UniformInt : public Uniform {
private:
    GLint i;
public:
    explicit UniformInt(GLint val);
    void load() override;
    void set(GLint val);
};

// Using unique_ptr for automatic memory management
using UniformPtr = std::unique_ptr<Uniform>;

class UniformLoader {
private:
    GLuint programId;
    std::map<std::string, UniformPtr> uniforms;
public:
    explicit UniformLoader(GLuint programId);
    ~UniformLoader();
    
    // Non-copyable
    UniformLoader(const UniformLoader&) = delete;
    UniformLoader& operator=(const UniformLoader&) = delete;
    
    void addUniform(std::string_view name, UniformPtr uniform);
    Uniform* get(std::string_view name) const;
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
