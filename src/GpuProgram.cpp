#include "GpuProgram.h"

GpuProgram::GpuProgram()
{
	id = glCreateProgram();
}

GpuProgram::~GpuProgram()
{
    glDeleteProgram(id);
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
