#ifndef _GPU_PROGRAM_H_
#define _GPU_PROGRAM_H_

#include "Common.h"
#include "Shader.h"

class Uniform {
protected:
	GLuint location;
public:
	virtual void load() = 0;
	GLuint getLocation();
	void setLocation(GLuint);
	virtual ~Uniform();
};

class UniformMat4 : public Uniform {
private:
	glm::mat4 matrix;
public:
	UniformMat4(glm::mat4&);
	void load();
	void set(glm::mat4&);
};

class UniformVec3 : public Uniform {
private:
	glm::vec3 vector;
public:
	UniformVec3(glm::vec3&);
	void load();
	void set(glm::vec3&);
};

class UniformInt : public Uniform {
private:
	GLint i;
public:
	UniformInt(int);
	void load();
	void set(GLint);
};

class UniformLoader {
private:
	GLuint programId;
	std::map<std::string, Uniform*> uniforms;
public:
	UniformLoader(GLuint);
	~UniformLoader();
	void addUniform(const char*, Uniform*);
	Uniform* get(const char*);
	void load();
};

class GpuProgram
{
public:
	GpuProgram();
	~GpuProgram();
	GLuint getId();
	void attachShader(Shader& _shader);
	void use();
	UniformLoader *uniformLoader;
private:
	GLuint id;
};

#endif
