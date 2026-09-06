with open('src/Renderer.cpp', 'r') as f:
    text = f.read()

import re

# 1. Remove checkForGLSLError and checkForGLError()
text = re.sub(r'void checkForGLSLError\(GLuint programId\)\s*\{[\s\S]*?\}\s*\}\s*\}', '', text)
text = text.replace('checkForGLError();', '')

# 2. ~Renderer()
text = text.replace('''    if (!occlusionQueries.empty())
    {
        glDeleteQueries(static_cast<GLsizei>(occlusionQueries.size()), occlusionQueries.data());
    }
    if (shadowMap != 0)
    {
        glDeleteTextures(1, &shadowMap);
    }''', '''    if (backend && backend->isOpenGL()) {
        if (!occlusionQueries.empty()) {
            glDeleteQueries(static_cast<GLsizei>(occlusionQueries.size()), occlusionQueries.data());
        }
        if (shadowMap != 0) {
            glDeleteTextures(1, &shadowMap);
        }
    }''')

# 3. addTexture wrappers
text = re.sub(
    r'glGenTextures\(1, textureId\);\s*glBindTexture\(GL_TEXTURE_2D, \*textureId\);[\s\S]*?glTexParameterf\(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso\);\s*\}',
    r'if (backend && backend->isOpenGL()) {\n        \g<0>\n    }',
    text
)

# 4. glGenQueries
text = re.sub(
    r'glGenQueries\(static_cast<GLsizei>\(occlusionQueries\.size\(\)\), occlusionQueries\.data\(\)\);',
    r'if (backend && backend->isOpenGL()) { glGenQueries(static_cast<GLsizei>(occlusionQueries.size()), occlusionQueries.data()); }',
    text
)

# 5. glGetQueryObjectuiv
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

with open('src/Renderer.cpp', 'w') as f:
    f.write(text)
