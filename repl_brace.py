import re

with open('src/Renderer.cpp', 'r') as f:
    text = f.read()

text = text.replace('}\n\nvoid Renderer::addTexture(std::string_view textureFileName, GLuint* textureId)', 'void Renderer::addTexture(std::string_view textureFileName, GLuint* textureId)')

with open('src/Renderer.cpp', 'w') as f:
    f.write(text)
