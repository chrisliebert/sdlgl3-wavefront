import re

with open('src/Renderer.cpp', 'r') as f:
    text = f.read()

# Replace the body of Renderer::addTexture(..., const Texture* texture)
new_body = '''void Renderer::addTexture(std::string_view /*textureFileName*/, GLuint* textureId, const Texture* texture)
{
    if (!textureId || !texture || !texture->isValid()) return;
    
    if (backend) {
        backend->addTexture(textureId, texture);
    }
}
'''

text = re.sub(
    r'void Renderer::addTexture\(std::string_view /\*textureFileName\*/, GLuint\* textureId, const Texture\* texture\)\s*\{[\s\S]*?\}\s*\}',
    new_body,
    text
)

with open('src/Renderer.cpp', 'w') as f:
    f.write(text)
