import re

with open('src/VulkanBackend.cpp', 'r') as f:
    text = f.read()

impl = '''void VulkanBackend::addTexture(GLuint* textureId, SDL_Surface* surface) {
#ifdef HAVE_VULKAN
    if (!surface) return;
    Texture tex;
    tex.width = surface->w;
    tex.height = surface->h;
    tex.data = std::shared_ptr<unsigned char[]>(new unsigned char[surface->w * surface->h * 4]);
    memcpy(tex.data.get(), surface->pixels, surface->w * surface->h * 4);
    tex.mode = 4; // Not used directly in Vulkan MVP but we set it
    addTexture(textureId, &tex);
#endif
}'''

text = re.sub(r'void VulkanBackend::addTexture\(GLuint\* textureId, SDL_Surface\* surface\) \{[\s\S]*?\}', impl, text)

with open('src/VulkanBackend.cpp', 'w') as f:
    f.write(text)
