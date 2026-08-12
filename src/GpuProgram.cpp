#include "GpuProgram.h"

// Uniform implementation
GLuint Uniform::getLocation()
{
    return location;
}

void Uniform::setLocation(GLuint _location)
{
    location = _location;
}

// UniformMat4 implementation
UniformMat4::UniformMat4(const glm::mat4& mat) : matrix(mat) {}

void UniformMat4::load()
{
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

void UniformMat4::set(const glm::mat4& mat)
{
    matrix = mat;
}

// UniformVec3 implementation
UniformVec3::UniformVec3(const glm::vec3& vec) : vector(vec) {}

void UniformVec3::load()
{
    glUniform3f(location, vector.x, vector.y, vector.z);
}

void UniformVec3::set(const glm::vec3& vec)
{
    vector = vec;
}

// UniformInt implementation
UniformInt::UniformInt(GLint val) : i(val) {}

void UniformInt::load()
{
    glUniform1i(location, i);
}

void UniformInt::set(GLint val)
{
    i = val;
}

// UniformLoader implementation
UniformLoader::UniformLoader(GLuint _programId)
    : programId(_programId)
{}

UniformLoader::~UniformLoader() = default;

void UniformLoader::addUniform(std::string_view name, UniformPtr uniform)
{
    if (uniform == nullptr) {
        fprintf(stderr, "Cannot add null uniform for '%s'\n", std::string(name).c_str());
        return;
    }
    
    GLuint loc = glGetUniformLocation(programId, std::string(name).c_str());
    if (loc == static_cast<GLuint>(-1)) {
        fprintf(stderr, "Uniform '%s' not found in shader program\n", std::string(name).c_str());
        return;
    }
    uniform->setLocation(loc);
    uniforms[std::string(name)] = std::move(uniform);
}

Uniform* UniformLoader::get(std::string_view name) const
{
    auto it = uniforms.find(std::string(name));
    if (it == uniforms.end()) {
        return nullptr;
    }
    return it->second.get();
}

void UniformLoader::load() const
{
    for (const auto& [key, uniform] : uniforms) {
        if (uniform) {
            uniform->load();
        }
    }
}

// GpuProgram implementation
GpuProgram::GpuProgram()
    : id(glCreateProgram())
    , uniformLoader(std::make_unique<UniformLoader>(id))
{}

GpuProgram::~GpuProgram()
{
    if (id != 0) {
        glDeleteProgram(id);
        id = 0;
    }
}

GLuint GpuProgram::getId() const
{
    return id;
}

void GpuProgram::attachShader(Shader& _shader)
{
    glAttachShader(id, _shader.getId());
}

void GpuProgram::use() const
{
    glUseProgram(id);
}
