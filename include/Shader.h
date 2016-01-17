#ifndef _SHADER_H_
#define _SHADER_H_

#include "Common.h"

class Shader
{
public:
    Shader(const char* _filePath);
    Shader(std::string& _filePath);
    Shader();
    ~Shader();
    void load(const char* _filePath);
    GLuint getId();
protected:
    GLuint id;
    const char* filePath;
    std::string shaderSrc;
};

class FragmentShader : public Shader
{
public:
    FragmentShader(const char* _filePath);
    FragmentShader(std::string& _filePath);
    ~FragmentShader();
protected:
    void createFragmentShader();
};

class VertexShader : public Shader
{
public:
    VertexShader(const char* _filePath);
    VertexShader(std::string& _filePath);
    ~VertexShader();
protected:
    void createVertexShader();
};

#endif
