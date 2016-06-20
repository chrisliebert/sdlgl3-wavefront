#include "Common.h"
#include "GpuProgram.h"

Uniform::~Uniform() {

}

void Uniform::setLocation(GLuint _location)
{
	location = _location;
}

GLuint Uniform::getLocation()
{
	return location;
}

UniformMat4::UniformMat4(glm::mat4& mat)
{
	matrix = mat;
}

void UniformMat4::load()
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

void UniformMat4::set(glm::mat4& mat)
{
	matrix = mat;
}

UniformVec3::UniformVec3(glm::vec3& vec)
{
	vector = vec;
}

void UniformVec3::load()
{
	glUniform3f(location, vector.x, vector.y, vector.z);
}

void UniformVec3::set(glm::vec3& vec)
{
	vector = vec;
}

UniformInt::UniformInt(GLint _i)
{
	i = _i;
}

void UniformInt::load()
{
	glUniform1i(location, i);
}

void UniformInt::set(GLint _i)
{
	i = _i;
}

UniformLoader::UniformLoader(GLuint _programId)
{
	programId = _programId;
}

UniformLoader::~UniformLoader()
{
    for (std::map<std::string, Uniform*>::iterator it=uniforms.begin(); it!=uniforms.end(); ++it) {
    	if(it->second != NULL) delete it->second;
    }
}

Uniform* UniformLoader::get(const char* lookup)
{
	std::string lookupStr(lookup);
	if(uniforms.find(lookupStr) == uniforms.end()) {
		std::cerr << "Uniform " << lookupStr << " does not exist." << std::endl;
		exit(5);
	}
	return uniforms[lookupStr];
}

void UniformLoader::load()
{
	for (std::map<std::string, Uniform*>::iterator it=uniforms.begin(); it!=uniforms.end(); ++it) {
		it->second->load();
	}
}

void UniformLoader::addUniform(const char* name, Uniform* uniform)
{
	std::string nameStr(name);
	uniform->setLocation(glGetUniformLocation(programId, name));
	uniforms[nameStr] = uniform;
}

GpuProgram::GpuProgram()
{
	id = glCreateProgram();
	uniformLoader = new UniformLoader(id);
}

GpuProgram::~GpuProgram()
{
    glDeleteProgram(id);
    if(uniformLoader != NULL) delete uniformLoader;
}

void GpuProgram::attachShader(Shader& _shader)
{
	glAttachShader(id, _shader.getId());
}

GLuint GpuProgram::getId()
{
	return id;
}

void GpuProgram::use()
{
	glUseProgram(id);
}
