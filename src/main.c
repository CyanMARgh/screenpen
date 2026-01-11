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

#include "utils.h"
#include "defer.h"
#include "ubo.h"

// settings
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

typedef struct {
    int32_t x, y;
} ivec2;

#include "shaders.h"


#define SHAPE_ERASER    0
#define SHAPE_BRUSH     1
#define SHAPE_BOX       2
int mode = SHAPE_BRUSH;

Shared_Shader_Data SHARED_SHADER_DATA;
GLuint FBO;
GLuint TEX_backbuffer;
// GLuint shader_line;
GLuint shader_stroke;
GLuint shader_copy_texture;
GLuint shader_cursor;
GLuint shader_box;
GLuint empty_VAO;
bool alpha_dim = false;

void draw_stroke() {
    printf("draw_stroke()\n");

    glBindFramebuffer(GL_FRAMEBUFFER, FBO); DEFER(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TEX_backbuffer, 0);
    glUseProgram(shader_stroke); DEFER(glUseProgram(0));

    glBindVertexArray(empty_VAO); DEFER(glBindVertexArray(0));
    //                        rectangle + circle
    glDrawArrays(GL_TRIANGLES, 0, 2 * 3 + 2 * 3);

    check_opengl_errors();
}

void draw_box() {
    printf("draw_box()\n");

    glBindFramebuffer(GL_FRAMEBUFFER, FBO); DEFER(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TEX_backbuffer, 0);
    glUseProgram(shader_box); DEFER(glUseProgram(0));

    glBindVertexArray(empty_VAO); DEFER(glBindVertexArray(0));
    glDrawArrays(GL_TRIANGLES, 0, 2 * 3);

    check_opengl_errors();
}

void show_backbuffer() {
    printf("show_backbuffer()\n");
    {
        float f = alpha_dim ? .2f : 0.f;
        glClearColor(f * .5f, f * .5f, f * .5f, f);
    }
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_BLEND); DEFER(glDisable(GL_BLEND));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(shader_copy_texture); DEFER(glUseProgram(0));

    glBindVertexArray(empty_VAO); DEFER(glBindVertexArray(0));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TEX_backbuffer);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    check_opengl_errors();
}

int main(void) {
    SHARED_SHADER_DATA = (Shared_Shader_Data){ 0 };
    SHARED_SHADER_DATA.color = 2;
    SHARED_SHADER_DATA.stroke_scale = 5;

    RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
    printf("setup pt. 1\n");
    hints->major = 4;
    hints->minor = 2;
    RGFW_setGlobalHints_OpenGL(hints);

    printf("setup pt. 2\n");

	RGFW_window* window = RGFW_createWindow("screenpen", SCR_WIDTH, SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT, RGFW_windowAllowDND | RGFW_windowCenter | RGFW_windowScaleToMonitor | RGFW_windowOpenGL | RGFW_windowTransparent);
    DEFER(RGFW_window_close(window));

    printf("setup pt. 3\n");
    SHARED_SHADER_DATA.window_size = (ivec2){window->w, window->h};

    printf("setup pt. 4\n");

    if (window == NULL) {
        printf("Failed to create RGFW window\n");
        return -1;
    }
    RGFW_window_setExitKey(window, RGFW_escape);
    printf("setup pt. 5\n");
    RGFW_window_makeCurrentContext_OpenGL(window);
    printf("setup pt. 6\n");

    int version = gladLoadGL(/*RGFW_getProcAddress_OpenGL*/);
    printf("setup pt. 7\n");
    if (version == 0) {
        printf("Failed to initialize OpenGL context\n");
        return -1;
    }

    printf("context setup done\n");
    check_opengl_errors();

    // shader_line = make_shader(line_vert, line_frag);
    // DEFER(glDeleteProgram(shader_line));

    shader_stroke = make_shader(vert_stroke, frag_stroke);
    DEFER(glDeleteProgram(shader_stroke));

    shader_cursor = make_shader(vert_cursor, frag_cursor);
    DEFER(glDeleteProgram(shader_stroke));

    shader_copy_texture = make_shader(copy_texture_vert, copy_texture_frag);
    DEFER(glDeleteProgram(shader_copy_texture));

    shader_box = make_shader(vert_box, frag_box);
    DEFER(glDeleteProgram(shader_copy_texture));

    glGenVertexArrays(1, &empty_VAO);
    DEFER(glDeleteVertexArrays(1, &empty_VAO));

    GLuint UBO_shared_shader_data = ubo_make(0, Shared_Shader_Data);
    DEFER(ubo_deinit(UBO_shared_shader_data));
    ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);

    { // make texture
        printf("creating backbuffer\n");
        glGenTextures(1, &TEX_backbuffer);
        glBindTexture(GL_TEXTURE_2D, TEX_backbuffer);
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

        check_opengl_errors();
    }
    DEFER(glDeleteTextures(1, &TEX_backbuffer));

    glGenFramebuffers(1, &FBO);
    DEFER(glDeleteBuffers(1, &FBO));

    {
        printf("creating FBO\n");

        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TEX_backbuffer, 0);

        glClearColor(0.f, 0.f, 0.f, .0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
        check_opengl_errors();
    }

    bool mouse_down = false;

    {
        ivec2 m;
        RGFW_window_getMouse(window, &m.x, &m.y);
        SHARED_SHADER_DATA.mouse_new = 
        SHARED_SHADER_DATA.mouse_old =
        SHARED_SHADER_DATA.mouse_stored = m;
    }

    glViewport(0, 0, window->w, window->h);

    printf("loop prepare done\n");
    check_opengl_errors();

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
                        SHARED_SHADER_DATA.color = key - '0';
                        ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);
                        if(mode == SHAPE_ERASER) {
                            mode = SHAPE_BRUSH; // disabling "eraser mode"
                        }
                    } else if(key == '0') {
                        SHARED_SHADER_DATA.color = 0;
                        mode = SHAPE_ERASER;
                        ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);
                    } else if(key == '-') {
                        SHARED_SHADER_DATA.stroke_scale = SHARED_SHADER_DATA.stroke_scale > 2 ? SHARED_SHADER_DATA.stroke_scale - 2 : 2;
                        ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);
                    } else if(key == '+' || key == '=') {
                        SHARED_SHADER_DATA.stroke_scale = SHARED_SHADER_DATA.stroke_scale < 30 ? SHARED_SHADER_DATA.stroke_scale + 2 : 30;
                        ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);
                    } else if(key == 'a') {
                        alpha_dim ^= true;
                    } else if(key == 'b') {
                        if(!SHARED_SHADER_DATA.color) { // disabling "eraser" color
                            SHARED_SHADER_DATA.color = 1;
                            ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);
                        }
                        mode = SHAPE_BOX;
                    } else if(key == 'p') {
                        if(!SHARED_SHADER_DATA.color) { // disabling "eraser" color
                            SHARED_SHADER_DATA.color = 1;
                            ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);
                        }
                        mode = SHAPE_BRUSH;
                    } else if(key == RGFW_F11) {
                        RGFW_window_setFullscreen(window, !RGFW_window_isFullscreen(window));
                    }
                    break;
                case RGFW_keyReleased:
                    // printf("Key released %c\n", event.key.sym);
                    break;
                case RGFW_mouseButtonPressed:
                    mouse_down = true;
                    ivec2 m;
                    RGFW_window_getMouse(window, &m.x, &m.y);

                    SHARED_SHADER_DATA.mouse_new = 
                    SHARED_SHADER_DATA.mouse_old =
                    SHARED_SHADER_DATA.mouse_stored = m;

                    ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);

                    if(mode == SHAPE_BRUSH || mode == SHAPE_ERASER) {
                        draw_stroke();
                    }
                    break;
                case RGFW_mouseButtonReleased:
                    mouse_down = false;
                    if(mode == SHAPE_BOX) {
                        draw_box();
                    }
                    // printf("Mouse Button Released %i\n", event.button.value);
                    break;
                case RGFW_mouseScroll:
                    // printf("Mouse Button scroll %f %f\n", (double)event.scroll.x, (double)event.scroll.y);
                    break;
                case RGFW_mousePosChanged:
                    printf("mouse position changed\n");
                    SHARED_SHADER_DATA.mouse_old = SHARED_SHADER_DATA.mouse_new;
                    SHARED_SHADER_DATA.mouse_new = (ivec2){event.mouse.x, event.mouse.y};
                    ubo_load(UBO_shared_shader_data, SHARED_SHADER_DATA);

                    // draw line
                    if(mouse_down && (mode == SHAPE_BRUSH || mode == SHAPE_ERASER)) {
                        draw_stroke();
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
                        glDeleteTextures(1, &TEX_backbuffer);
                        glGenTextures(1, &TEX_backbuffer);
                        glBindTexture(GL_TEXTURE_2D, TEX_backbuffer);
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
                    // for(int i = 0; i < event.drop.count; i++) {
                        // printf("dropped : %s\n", event.drop.files[i]);
                    // }
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
        }
        check_opengl_errors();

        show_backbuffer();

        if(!mouse_down || mode == SHAPE_ERASER) {
            glUseProgram(shader_cursor); DEFER(glUseProgram(0));
            glBindVertexArray(empty_VAO); DEFER(glBindVertexArray(0));

            glDrawArrays(GL_TRIANGLES, 0, 6);
        } else {
            if(mode == SHAPE_BRUSH) {
                // nothing
            } else if(mode == SHAPE_BOX) {
                glUseProgram(shader_box); DEFER(glUseProgram(0));
                glBindVertexArray(empty_VAO); DEFER(glBindVertexArray(0));

                glDrawArrays(GL_TRIANGLES, 0, 6);                
            }
        }
        check_opengl_errors();

        RGFW_window_swapBuffers_OpenGL(window);

        check_opengl_errors();
    }

    return 0;
}
