import re

with open('src/VulkanBackend.cpp', 'r') as f:
    text = f.read()

# Replace the memory mapping logic in addTexture(const Texture*)
replacement = '''    VkDeviceSize imageSize = texture->width * texture->height * 4; // We use RGBA in Vulkan
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vkhelpers::createBuffer(m_physicalDevice, m_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    
    if (texture->bpp == 3 || texture->mode == 3) {
        // Convert RGB to RGBA
        const uint8_t* src = texture->data.get();
        uint8_t* dst = static_cast<uint8_t*>(data);
        for (size_t i = 0; i < texture->width * texture->height; i++) {
            dst[i*4 + 0] = src[i*3 + 0];
            dst[i*4 + 1] = src[i*3 + 1];
            dst[i*4 + 2] = src[i*3 + 2];
            dst[i*4 + 3] = 255;
        }
    } else {
        // Assume 4 bpp
        memcpy(data, texture->data.get(), static_cast<size_t>(imageSize));
    }
    
    vkUnmapMemory(m_device, stagingBufferMemory);'''

# Find the old code and replace it
text = re.sub(
    r'VkDeviceSize imageSize = texture->width \* texture->height \* 4; // Assuming RGBA\s*VkBuffer stagingBuffer;\s*VkDeviceMemory stagingBufferMemory;\s*vkhelpers::createBuffer.*?;\s*void\* data;\s*vkMapMemory.*?;\s*memcpy\(data, texture->data\.get\(\), static_cast<size_t>\(imageSize\)\);\s*vkUnmapMemory.*?;',
    replacement,
    text,
    flags=re.DOTALL
)

with open('src/VulkanBackend.cpp', 'w') as f:
    f.write(text)
