#ifndef _UBO_H
#define _UBO_H

#define ubo_make(slot, T) _ubo_make(slot, sizeof(T))

GLuint _ubo_make(int slot, size_t size) {
    printf("creating UBO\n");
    GLuint UBO;
    {
        glGenBuffers(1, &UBO);
        glBindBuffer(GL_UNIFORM_BUFFER, UBO);
            glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, slot, UBO);

    check_opengl_errors();
    return UBO;
}

void ubo_deinit(GLuint UBO) {    
    glDeleteBuffers(1, &UBO);
}

#define ubo_load(UBO, object) _ubo_load(UBO, &(object), sizeof(object))

void _ubo_load(GLuint UBO, void* data, size_t size) {
    printf("updating UBO data (%d)\n", UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    check_opengl_errors();
}

#endif