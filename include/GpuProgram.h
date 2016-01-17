#ifndef _GPU_PROGRAM_H_
#define _GPU_PROGRAM_H_

#include "Common.h"
#include "Shader.h"

class GpuProgram
{
public:
	GpuProgram();
	~GpuProgram();
	GLuint getId();
	void attachShader(Shader& _shader);
	void use();
private:
	GLuint id;
};

#endif