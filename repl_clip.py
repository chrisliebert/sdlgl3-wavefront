import re

with open('src/VulkanBackend.cpp', 'r') as f:
    text = f.read()

clip_code = '''        glm::mat4 clip(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f,-1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.0f, 0.0f, 0.5f, 1.0f);
        pc.projection = clip * m_projectionMatrix;'''

text = re.sub(
    r'pc\.projection = m_projectionMatrix;\s*pc\.projection\[1\]\[1\] \*= -1\.0f;',
    clip_code,
    text
)

# And re-enable backface culling since it was probably right!
text = text.replace('rasterizer.cullMode = VK_CULL_MODE_NONE;', 'rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;')

with open('src/VulkanBackend.cpp', 'w') as f:
    f.write(text)
