import re

with open('src/VulkanBackend.cpp', 'r') as f:
    text = f.read()

impl = '''void VulkanBackend::addTexture(GLuint* textureId, SDL_Surface* surface) {
#ifdef HAVE_VULKAN
    if (!surface) return;
    Texture tex;
    tex.width = surface->w;
    tex.height = surface->h;
    tex.data = std::shared_ptr<uint8_t>(new uint8_t[surface->w * surface->h * 4], std::default_delete<uint8_t[]>());
    memcpy(tex.data.get(), surface->pixels, surface->w * surface->h * 4);
    tex.mode = 4; // Not used directly in Vulkan MVP but we set it
    addTexture(textureId, &tex);
#endif
}'''

text = re.sub(r'void VulkanBackend::addTexture\(GLuint\* textureId, SDL_Surface\* surface\) \{\}', impl, text)

# ALSO fix my call in initialize!
text = text.replace('addTexture(surface, &defaultTexId);', 'addTexture(&defaultTexId, surface);')

with open('src/VulkanBackend.cpp', 'w') as f:
    f.write(text)
