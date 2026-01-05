#ifndef _UTILS_H
#define _UTILS_H

void check_opengl_errors() {
    GLenum code = glGetError();
    const char* error = NULL;
    switch(code) {
    case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
    case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
    case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
    case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
    case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
    case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
    case GL_NO_ERROR:
        return;
    }
    if(error) {
        printf("OpenGL error: %s\n", error);
    } else {
        printf("OpenGL error: %d\n", code);        
    }
    exit(1);
}


#endif