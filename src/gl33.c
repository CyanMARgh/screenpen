#define RGFW_OPENGL
// #define RGFW_USE_XDL // feel free to remove this line if you don't want to use XDL (-lX11 -lXrandr -lGLX will be required)
#define RGFW_ALLOC_DROPFILES
#define RGFW_IMPLEMENTATION
#define RGFW_PRINT_ERRORS
#define RGFW_DEBUG
#define GL_SILENCE_DEPRECATION
#include "glad.h"
#include "RGFW.h"
// #include <GL/gl.h>

// #define RGL_LOAD_IMPLEMENTATION
// #include "rglLoad.h"

// #include 

#define MULTILINE_STR(...) #__VA_ARGS__

#include <stdbool.h>

#include "defer.h"

// settings
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

typedef struct {
    int32_t x, y;
} ivec2;

#define SHAPE_ERASER    0
#define SHAPE_BRUSH     1
#define SHAPE_BOX       2


#include "shaders.h"

Shared_Shader_Data SHARED_SHADER_DATA;

GLuint compile_shader_part(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    // check for shader compile errors
    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        printf("shader compilation failed:\n%s\n", info_log);
        exit(1);
    }

    return shader;
}

GLuint make_shader(const char* src_vert, const char* src_frag) {
    printf("compiling vertex shader\n");
    GLuint vertex_shader = compile_shader_part(src_vert, GL_VERTEX_SHADER);
    printf("compiling fragment shader\n");
    GLuint fragment_shader = compile_shader_part(src_frag, GL_FRAGMENT_SHADER);

    unsigned int shader_program = glCreateProgram();
    {
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);

        int success;
        char info_log[512];
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_program, 512, NULL, info_log);
            printf("shader linking failed\n%s\n", info_log);
            exit(1);
        }

    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shader_program;
}

int main(void) {
    SHARED_SHADER_DATA = (Shared_Shader_Data){ 0 };
    SHARED_SHADER_DATA.color = 2;
    SHARED_SHADER_DATA.stroke_scale = 5;

    RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
    hints->major = 4;
    hints->minor = 2;
    RGFW_setGlobalHints_OpenGL(hints);

	RGFW_window* window = RGFW_createWindow("LearnOpenGL", SCR_WIDTH, SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT, RGFW_windowAllowDND | RGFW_windowCenter | RGFW_windowScaleToMonitor | RGFW_windowOpenGL | RGFW_windowTransparent);
    DEFER(RGFW_window_close(window));
    SHARED_SHADER_DATA.window_size = (ivec2){window->w, window->h};

    RGFW_window_setFullscreen(window, true);
    if (window == NULL) {
        printf("Failed to create RGFW window\n");
        return -1;
    }
    RGFW_window_setExitKey(window, RGFW_escape);
    RGFW_window_makeCurrentContext_OpenGL(window);


    int version = gladLoadGL(/*RGFW_getProcAddress_OpenGL*/);
    if (version == 0) {
        printf("Failed to initialize OpenGL context\n");
        return -1;
    }


    GLuint test_shader = make_shader(vertexShaderSource, fragmentShaderSource);
    DEFER(glDeleteProgram(test_shader));

    GLuint line_shader = make_shader(line_vert, line_frag);
    DEFER(glDeleteProgram(line_shader));

    GLuint shader_stroke = make_shader(vert_stroke, frag_stroke);
    DEFER(glDeleteProgram(shader_stroke));

    GLuint shader_copy_texture = make_shader(copy_texture_vert, copy_texture_frag);
    DEFER(glDeleteProgram(shader_copy_texture));

    float vertices[] = { 0.5f,  0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f, -0.5f,  0.5f, 0.0f };
    uint32_t indices[] = { 0, 1, 3, 1, 2, 3 };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    DEFER(
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    );

    glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    GLuint empty_VAO;
    glGenVertexArrays(1, &empty_VAO);
    DEFER(glDeleteVertexArrays(1, &empty_VAO));


    GLuint UBO_shared_shader_data;
    {
        glGenBuffers(1, &UBO_shared_shader_data);
        glBindBuffer(GL_UNIFORM_BUFFER, UBO_shared_shader_data);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(Shared_Shader_Data), NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO_shared_shader_data);
    DEFER(glDeleteBuffers(1, &UBO_shared_shader_data));

    { // update
        glBindBuffer(GL_UNIFORM_BUFFER, UBO_shared_shader_data);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SHARED_SHADER_DATA), &SHARED_SHADER_DATA);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    // { // map UBO (gpu) memory
    //     glBindBuffer(GL_UNIFORM_BUFFER, UBO_shared_shader_data);
    //     SHARED_SHADER_DATA = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    //     memset(SHARED_SHADER_DATA, 0, sizeof(Shared_Shader_Data));
    // }
    // DEFER(glUnmapBuffer(GL_UNIFORM_BUFFER));


    GLuint temporal_texture;
    { // make texture
        glGenTextures(1, &temporal_texture);
        glBindTexture(GL_TEXTURE_2D, temporal_texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                window->w, window->h,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                NULL
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    DEFER(glDeleteTextures(1, &temporal_texture));

    GLuint FBO = 0;
    glGenFramebuffers(1, &FBO);
    DEFER(glDeleteBuffers(1, &FBO));

    {
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temporal_texture, 0);

        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool mouse_down = false;

    bool alpha_dim = false;

    {
        ivec2 m;
        RGFW_window_getMouse(window, &m.x, &m.y);
        SHARED_SHADER_DATA.mouse_new = m;
        SHARED_SHADER_DATA.mouse_old = m;
    }

    glViewport(0, 0, window->w, window->h);

    while(RGFW_window_shouldClose(window) == RGFW_FALSE) {
        RGFW_event event;
        RGFW_waitForEvent(RGFW_eventWaitNext);

        while(RGFW_window_checkEvent(window, &event)) {
            switch (event.type) {
                case RGFW_quit:
                    // printf("window closed\n");
                    break;
                case RGFW_keyPressed:
                    int key = event.key.value;
                    printf("Key pressed %c\n", event.key.value);
                    if(key >= '1' && key <= '9') {
                        SHARED_SHADER_DATA.color = key - '1';

                        { // update UBO
                            glBindBuffer(GL_UNIFORM_BUFFER, UBO_shared_shader_data);
                                glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SHARED_SHADER_DATA), &SHARED_SHADER_DATA);
                            glBindBuffer(GL_UNIFORM_BUFFER, 0);
                        }
                    } else if(key == '-') {
                        SHARED_SHADER_DATA.stroke_scale = SHARED_SHADER_DATA.stroke_scale > 2 ? SHARED_SHADER_DATA.stroke_scale - 2 : 2;
                    } else if(key == '+' || key == '=') {
                        SHARED_SHADER_DATA.stroke_scale = SHARED_SHADER_DATA.stroke_scale < 30 ? SHARED_SHADER_DATA.stroke_scale + 2 : 30;
                    } else if(key == 'a') {
                        alpha_dim ^= true;
                    }
                    break;
                case RGFW_keyReleased:
                    // printf("Key released %c\n", event.key.sym);
                    break;
                case RGFW_mouseButtonPressed:
                    mouse_down = true;
                    ivec2 m;
                    RGFW_window_getMouse(window, &m.x, &m.y);
                    SHARED_SHADER_DATA.mouse_stored = m;
                    break;
                case RGFW_mouseButtonReleased:
                    mouse_down = false;

                    SHARED_SHADER_DATA.mouse_new = SHARED_SHADER_DATA.mouse_old;
                    { // update UBO
                        glBindBuffer(GL_UNIFORM_BUFFER, UBO_shared_shader_data);
                            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SHARED_SHADER_DATA), &SHARED_SHADER_DATA);
                        glBindBuffer(GL_UNIFORM_BUFFER, 0);
                    }

                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temporal_texture, 0);
                        {
                            glUseProgram(shader_stroke);

                            glUniformBlockBinding(line_shader, glGetUniformBlockIndex(line_shader, "Shared_Shader_Data"), 0);

                            glBindVertexArray(empty_VAO);

                            glDrawArrays(GL_TRIANGLES, 0, 2 * 3 + 6 * 3);

                            glBindVertexArray(0);
                            glUseProgram(0);
                        }
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    }

                    // printf("Mouse Button Released %i\n", event.button.value);
                    break;
                case RGFW_mouseScroll:
                    // printf("Mouse Button scroll %f %f\n", (double)event.scroll.x, (double)event.scroll.y);
                    break;
                case RGFW_mousePosChanged:
                    SHARED_SHADER_DATA.mouse_old = SHARED_SHADER_DATA.mouse_new;
                    SHARED_SHADER_DATA.mouse_new = (ivec2){event.mouse.x, event.mouse.y};

                    { // update UBO
                        glBindBuffer(GL_UNIFORM_BUFFER, UBO_shared_shader_data);
                            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SHARED_SHADER_DATA), &SHARED_SHADER_DATA);
                        glBindBuffer(GL_UNIFORM_BUFFER, 0);
                    }

                    // TODO do not draw lines. just draw proper polygons already

                    // draw line
                    if(mouse_down)
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temporal_texture, 0);

                        {
                            glUseProgram(shader_stroke);

                            glUniformBlockBinding(line_shader, glGetUniformBlockIndex(line_shader, "Shared_Shader_Data"), 0);

                            glBindVertexArray(empty_VAO);

                            glDrawArrays(GL_TRIANGLES, 0, 2 * 3 + 2 * 2 * 3);

                            glBindVertexArray(0);
                            glUseProgram(0);
                        }
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    }
                    break;
                case RGFW_windowMoved:
                    // printf("window moved %i %i\n", window->x, window->y);
                    break;
                case RGFW_windowResized:
                    printf("window resize %i %i\n", window->w, window->h);
                    glViewport(0, 0, window->w, window->h);
                    SHARED_SHADER_DATA.window_size = (ivec2){window->w, window->h};

                    { // resize texture
                        glDeleteTextures(1, &temporal_texture);
                        glGenTextures(1, &temporal_texture);
                        glBindTexture(GL_TEXTURE_2D, temporal_texture);
                            glTexImage2D(
                                GL_TEXTURE_2D,
                                0,
                                GL_RGBA,
                                window->w, window->h,
                                0,
                                GL_RGBA,
                                GL_UNSIGNED_BYTE,
                                NULL
                            );
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        glBindTexture(GL_TEXTURE_2D, 0);
                    }

                    break;
                case RGFW_windowMaximized:
                  // printf("window maximized %i %i\n", window->w, window->h);
                    break;
                case RGFW_windowMinimized:
                    // printf("window minimized\n");
                    break;
                case RGFW_windowRestored:
                    // printf("window restored %i %i\n", window->w, window->h);
                    break;
                case RGFW_focusIn:
                    // printf("Focused\n");
                    break;
                case RGFW_focusOut:
                    // printf("Unfocused\n");
                    break;
                case RGFW_mouseEnter:
                    // printf("Mouse Entered %i %i\n", event.mouse.x, event.mouse.y);
                    break;
                case RGFW_mouseLeave:
                    // printf("Mouse left\n");
                    break;
                case RGFW_windowRefresh:
                    // printf("Refresh\n");
                    break;
                case RGFW_dataDrop: {
                    for(int i = 0; i < event.drop.count; i++) {
                        // printf("dropped : %s\n", event.drop.files[i]);
                    }
                    break;
                }
                case RGFW_dataDrag:
                    // printf("Drag : %i %i\n", event.drag.x, event.drag.y);
                    break;
                case RGFW_scaleUpdated:
                    // printf("Scale Updated : %f %f\n", (double)event.scale.x, (double)event.scale.y);
                    break;
                default:
                    break;
            }
    
            // printf("(%6d) mouse %i %i\n", framecount++, event.mouse.x, event.mouse.y);

        }

        glClearColor(0.0f, 0.f, 0.f, alpha_dim ? .1f : 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
        {
            glUseProgram(shader_copy_texture);

            glBindVertexArray(empty_VAO);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, temporal_texture);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindVertexArray(0);

            glUseProgram(0);
        }
        glDisable(GL_BLEND);

        {
            glUseProgram(line_shader);

            // TODO cleanup
            glUniformBlockBinding(line_shader, 0, 0);

            glBindVertexArray(empty_VAO);
            glPointSize(SHARED_SHADER_DATA.stroke_scale);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);
            glUseProgram(0);
        }

        RGFW_window_swapBuffers_OpenGL(window);

        GLenum code = glGetError();
        assert(code == GL_NO_ERROR);
    }

    return 0;
}
