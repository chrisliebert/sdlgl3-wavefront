#include "Shader.h"

void FragmentShader::createFragmentShader()
{
    id = glCreateShader(GL_FRAGMENT_SHADER);
    const char* src = shaderSrc.c_str();
    glShaderSource(id, 1, &src, 0);
    glCompileShader(id);
    GLint shaderCompiled;

    glGetShaderiv(id, GL_COMPILE_STATUS, &shaderCompiled);

    if(shaderCompiled == GL_FALSE)
    {
        int infologLength = 0;

        int charsWritten  = 0;
        char *infoLog;

        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infologLength);
        std::string log = "";
        if (infologLength > 0)
        {
            infoLog = (char *)malloc(infologLength);
            glGetShaderInfoLog(id, infologLength, &charsWritten, infoLog);
            log = infoLog;
            free(infoLog);
        }
        std::cerr << "The shader " << filePath << " failed to compile: " << log << std::endl;
    }
}

void VertexShader::createVertexShader()
{
    id = glCreateShader(GL_VERTEX_SHADER);
    const char* src = shaderSrc.c_str();
    glShaderSource(id, 1, &src, 0);
    glCompileShader(id);
    GLint shaderCompiled;

    glGetShaderiv(id, GL_COMPILE_STATUS, &shaderCompiled);

    if(shaderCompiled == GL_FALSE)
    {
        int infologLength = 0;

        int charsWritten  = 0;
        char *infoLog;

        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infologLength);
        std::string log = "";
        if (infologLength > 0)
        {
            infoLog = (char *)malloc(infologLength);
            glGetShaderInfoLog(id, infologLength, &charsWritten, infoLog);
            log = infoLog;
            free(infoLog);
        }
        std::cerr << "The shader " << filePath << " failed to compile: " << log << std::endl;

    }
}

void Shader::load(const char* _filePath)
{
    shaderSrc = "";
    filePath = _filePath;
    std::ifstream fileStream(filePath);
    if (fileStream.is_open())
    {
        std::string line;
        while (std::getline(fileStream, line))
        {
            shaderSrc += line;
            shaderSrc += "\n";
        }
        fileStream.close();
    }
    else
    {
        std::cerr << "Unable to load shader source" << std::endl;
    }
}

Shader::Shader(const char* _filePath)
{
    load(_filePath);
}

Shader::Shader(std::string& _filePath)
{
    load(_filePath.c_str());
}

Shader::Shader()
{

}

Shader::~Shader()
{
    glDeleteShader(id);
}

GLuint Shader::getId()
{
    return id;
}

FragmentShader::FragmentShader(const char* _filePath)
{
    load(_filePath);
    createFragmentShader();
}

FragmentShader::FragmentShader(std::string& _filePath)
{
    load(_filePath.c_str());
    createFragmentShader();
}

FragmentShader::~FragmentShader()
{

}

VertexShader::VertexShader(const char* _filePath)
{
    load(_filePath);
    createVertexShader();
}

VertexShader::VertexShader(std::string& _filePath) : Shader(_filePath.c_str())
{
    load(_filePath.c_str());
    createVertexShader();
}

VertexShader::~VertexShader()
{

}
