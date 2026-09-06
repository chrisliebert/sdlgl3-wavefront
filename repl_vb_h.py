import re

with open('include/VulkanBackend.h', 'r') as f:
    text = f.read()

members = '''    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    VkFormat findDepthFormat();
    void createDepthResources();'''

text = text.replace('    VkRenderPass m_renderPass = VK_NULL_HANDLE;', members)

with open('include/VulkanBackend.h', 'w') as f:
    f.write(text)
