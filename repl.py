with open('src/Renderer.cpp', 'r') as f:
    text = f.read()

replacement = '''void Renderer::resolveTextures()
{
    for (auto& node : sceneNodes)
    {
        if (node.material[0] == '\\0') continue;
        
        std::string materialName(node.material);
        auto it = materials.find(materialName);
        if (it != materials.end())
        {
            const Material& mat = it->second;
            
            if (mat.hasDiffuseTexture())
            {
                addTexture(mat.diffuseTexName, &node.diffuseTextureId);
            }
            if (mat.normalTexName[0] != '\\0')
            {
                addTexture(mat.normalTexName, &node.normalTextureId);
            }
            if (mat.specularTexName[0] != '\\0')
            {
                addTexture(mat.specularTexName, &node.specularTextureId);
            }
        }
    }
}

void Renderer::bufferToGpu(Camera& camera, bool loadCachedScene)
{
    resolveTextures();'''

text = text.replace('void Renderer::bufferToGpu(Camera& camera, bool loadCachedScene)\n{', replacement)

with open('src/Renderer.cpp', 'w') as f:
    f.write(text)
