with open('src/OpenGLBackend.cpp', 'a') as f:
    f.write('''
void OpenGLBackend::addTexture(GLuint* textureId, const struct Texture* texture) {
    if (!textureId || !texture || !texture->isValid()) return;
    
    glGenTextures(1, textureId);
    glBindTexture(GL_TEXTURE_2D, *textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, texture->mode, 
                 static_cast<GLsizei>(texture->width), static_cast<GLsizei>(texture->height),
                 0, texture->mode, GL_UNSIGNED_BYTE, texture->data.get());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Anisotropic filtering
    GLfloat maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    if (maxAniso > 0.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
    }
}
''')
