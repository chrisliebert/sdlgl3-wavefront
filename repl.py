import re

with open('src/VulkanBackend.cpp', 'r') as f:
    text = f.read()

# Let's see if we can create a default texture.
# In initialize(), after createSyncObjects(), we can do:
default_tex_code = '''
    createSyncObjects();

    // Create a default 1x1 white texture
    SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
    if (surface) {
        uint32_t* pixels = (uint32_t*)surface->pixels;
        *pixels = 0xFFFFFFFF; // White
        uint32_t defaultTexId = 0;
        addTexture(surface, &defaultTexId);
        SDL_DestroySurface(surface);
    }
'''
text = text.replace('createSyncObjects();', default_tex_code)

# Then in submit:
bind_code = '''
        VkDescriptorSet ds = VK_NULL_HANDLE;
        if (cmd.node->diffuseTextureId > 0 && cmd.node->diffuseTextureId <= m_textures.size()) {
            ds = m_textures[cmd.node->diffuseTextureId - 1].descriptorSet;
        } else if (!m_textures.empty()) {
            ds = m_textures[0].descriptorSet;
        }
        
        if (ds != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &ds, 0, nullptr);
        }
'''

# Let's replace the existing bind logic
text = re.sub(
    r'if\s*\(cmd\.node->diffuseTextureId\s*>\s*0\s*&&\s*cmd\.node->diffuseTextureId\s*<=\s*m_textures\.size\(\)\)\s*\{\s*VkDescriptorSet\s*ds\s*=\s*m_textures\[cmd\.node->diffuseTextureId\s*-\s*1\]\.descriptorSet;\s*vkCmdBindDescriptorSets\(m_commandBuffer,\s*VK_PIPELINE_BIND_POINT_GRAPHICS,\s*m_pipelineLayout,\s*0,\s*1,\s*&ds,\s*0,\s*nullptr\);\s*\}',
    bind_code,
    text
)

with open('src/VulkanBackend.cpp', 'w') as f:
    f.write(text)
