with open('src/Renderer.cpp', 'r') as f:
    text = f.read()

import re

# 1. glGenQueries
text = re.sub(
    r'glGenQueries\(static_cast<GLsizei>\(occlusionQueries\.size\(\)\), occlusionQueries\.data\(\)\);',
    r'if (backend && backend->isOpenGL()) { glGenQueries(static_cast<GLsizei>(occlusionQueries.size()), occlusionQueries.data()); }',
    text
)

# 2. glGetQueryObjectuiv
text = re.sub(
    r'glGetQueryObjectuiv\(query, GL_QUERY_RESULT_AVAILABLE, &available\);',
    r'if (backend && backend->isOpenGL()) { glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available); }',
    text
)

text = re.sub(
    r'glGetQueryObjectuiv\(query, GL_QUERY_RESULT, &samples\);',
    r'if (backend && backend->isOpenGL()) { glGetQueryObjectuiv(query, GL_QUERY_RESULT, &samples); }',
    text
)

# 3. GL_RGB etc are macros, not function calls, so they are fine!

# 4. addTexture(image) and addTexture(texture)
# I need to wrap the whole glGenTextures to maxAniso thing
text = re.sub(
    r'glGenTextures\(1, textureId\);\s*glBindTexture\(GL_TEXTURE_2D, \*textureId\);[\s\S]*?glTexParameterf\(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso\);\s*\}',
    r'if (backend && backend->isOpenGL()) {\n        \g<0>\n    }',
    text
)


with open('src/Renderer.cpp', 'w') as f:
    f.write(text)
