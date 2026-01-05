#ifndef _SHADERS_H
#define _SHADERS_H

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

void shader_set_block_bindings(GLuint shader);

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

    shader_set_block_bindings(shader_program);

    return shader_program;
}

typedef struct {
    ivec2 mouse_old;
    ivec2 mouse_new;
    ivec2 mouse_stored;
    ivec2 window_size;

    int32_t active_shape;
    int32_t color;

    int32_t stroke_scale;
} Shared_Shader_Data;

#define SHARED_SHADER_DATA_RESERVED_BINDING_POINT 0

void shader_set_block_bindings(GLuint shader) {
    GLint id = glGetUniformBlockIndex(shader, "Shared_Shader_Data");
    if(id != -1) {
        glUniformBlockBinding(shader, id, SHARED_SHADER_DATA_RESERVED_BINDING_POINT);
    }
}

const char *vertexShaderSource = MULTILINE_STR(
\x23version 420 core\n
layout(location = 0) in vec3 a_pos;

void main() {
    gl_Position = vec4(a_pos.xyz, 1);
}
);

const char *fragmentShaderSource = MULTILINE_STR(
\x23version 420 core\n
out vec4 o_color;

vec4 pma(vec4 c) {
    return vec4(c.rgb * c.a, c.a);
}

void main() {
    o_color = pma(vec4(1, 0, 0, 0.1));
}
);



const char *line_vert = MULTILINE_STR(
    \x23version 420 core\n

    layout(location = 0) in vec3 a_pos;

    const vec3 A = vec3(0, 0, 0);
    const vec3 B = vec3(1, 1, 0);

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    void main() {
        ivec2 ip = gl_VertexID == 0 ? mouse_new : mouse_old;
        vec2 p = (vec2(ip) / vec2(window_size)) * vec2(2, -2) + vec2(-1, 1);
        gl_Position = vec4(p, 0, 1);
    }
);
// TODO hardcoded size

const char *line_frag = MULTILINE_STR(
    \x23version 420 core\n

    out vec4 o_color;

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    vec3 colors_map[] = vec3[](
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(0, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 1, 1),
        vec3(1, 0, 1),
        vec3(1, 0.5, 0),
        vec3(1, 1, 1)
    );

    void main() {
        o_color = vec4(colors_map[color], 1);
    }
);

const char *copy_texture_vert = MULTILINE_STR(
    \x23version 420 core\n

    const vec2 points[6] = vec2[6](
        vec2(1, 1), vec2(1, 0), vec2(0, 0),
        vec2(1, 1), vec2(0, 0), vec2(0, 1)
    );

    out vec2 vo_uv;

    void main() {
        vec2 uv = points[gl_VertexID];

        gl_Position = vec4(uv * 2 - 1, 0, 1);
        vo_uv = uv;
    }
);

const char *copy_texture_frag = MULTILINE_STR(
    \x23version 420 core\n

    layout(binding = 0) uniform sampler2D u_tex;
    in vec2 vo_uv;
    out vec4 o_color;

    void main() {
        o_color = texture(u_tex, vo_uv);
    }
);

const char *vert_stroke = MULTILINE_STR(
    \x23version 420 core\n

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    vec2 rot90(vec2 v) { // counter-clockwise
        return vec2(-v.y, v.x);
    }
    
    out vec2 vo_uv;

    void main() {
        int id = gl_VertexID;
        vec2 P1 = vec2(mouse_new);
        vec2 P2 = vec2(mouse_old);

        vec2 dP = P2 - P1;
        vec2 dR = normalize(dP) * stroke_scale;
        vec2 dR90 = rot90(dR);

        vec2 A = P2 + dR90;
        vec2 B = P2 - dR90;
        vec2 C = P1 - dR90;
        vec2 D = P1 + dR90;

        vec2 P2A = P2 + vec2(-stroke_scale,  stroke_scale);
        vec2 P2B = P2 + vec2( stroke_scale,  stroke_scale);
        vec2 P2C = P2 + vec2( stroke_scale, -stroke_scale);
        vec2 P2D = P2 + vec2(-stroke_scale, -stroke_scale);

        vec2 vertices[] = vec2[](
            A, B, C,
            A, C, D,

            P2A, P2B, P2C,
            P2A, P2C, P2D
        );

        vec2 uvs[] = vec2[](
            vec2(0), vec2(0), vec2(0), // central rectangle
            vec2(0), vec2(0), vec2(0),

            vec2(-1, 1), vec2(1, 1), vec2(1, -1), // finishing circle
            vec2(-1, 1), vec2(1, -1), vec2(-1, -1)
        );

        vec2 p;
        p = vertices[id];
        vo_uv = uvs[id];

        gl_Position = vec4((p / vec2(window_size)) * vec2(2, -2) + vec2(-1, 1), 0, 1);
    }
);


const char *frag_stroke = MULTILINE_STR(
    \x23version 420 core\n

    uniform vec4 u_color = vec4(1, 0, 0, 1);
    out vec4 o_color;

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    vec3 colors_map[] = vec3[](
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(0, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 1, 1),
        vec3(1, 0, 1),
        vec3(1, 0.5, 0),
        vec3(1, 1, 1)
    );

    in vec2 vo_uv;

    void main() {
        if(vo_uv.x * vo_uv.x + vo_uv.y * vo_uv.y > 1) {
            discard;
        }
        o_color = vec4(colors_map[color], 1);
    }
);


const char *vert_cursor = MULTILINE_STR(
    \x23version 420 core\n

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    vec2 rot90(vec2 v) { // counter-clockwise
        return vec2(-v.y, v.x);
    }
    
    out vec2 vo_uv;

    void main() {
        int id = gl_VertexID;
        vec2 P0 = vec2(mouse_new);

        vec2 uvs[] = vec2[](
            vec2(-1, 1), vec2(1, 1), vec2(1, -1),
            vec2(-1, 1), vec2(1, -1), vec2(-1, -1)
        );


        vec2 p = P0 + uvs[id] * float(stroke_scale);
        vo_uv = uvs[id] * float(stroke_scale);

        gl_Position = vec4((p / vec2(window_size)) * vec2(2, -2) + vec2(-1, 1), 0, 1);
    }
);


const char *frag_cursor = MULTILINE_STR(
    \x23version 420 core\n

    out vec4 o_color;

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    vec3 colors_map[] = vec3[](
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(0, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 1, 1),
        vec3(1, 0, 1),
        vec3(1, 0.5, 0),
        vec3(1, 1, 1)
    );

    in vec2 vo_uv;

    void main() {
        float r2 = vo_uv.x * vo_uv.x + vo_uv.y * vo_uv.y;
        float rmin = max(stroke_scale - 4, 0);
        float rmax = stroke_scale;
        if(r2 < rmin * rmin || r2 > rmax * rmax) {
            discard;
        }
        o_color = vec4(colors_map[color], 1);
    }
);


const char *vert_box = MULTILINE_STR(
    \x23version 420 core\n

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    vec2 rot90(vec2 v) { // counter-clockwise
        return vec2(-v.y, v.x);
    }
    
    out vec2 vo_uv;

    void main() {
        int id = gl_VertexID;

        vec2 A = vec2(mouse_stored);
        vec2 B = vec2(mouse_new);

        vec2 uvs[] = vec2[](
            vec2(0, 1), vec2(1, 1), vec2(1, 0),
            vec2(0, 1), vec2(1, 0), vec2(0, 0)
        );

        vec2 p = A + (B - A) * uvs[id];
        vo_uv = p;
        gl_Position = vec4((p / vec2(window_size)) * vec2(2, -2) + vec2(-1, 1), 0, 1);
    }
);


const char *frag_box = MULTILINE_STR(
    \x23version 420 core\n

    out vec4 o_color;

    layout(std140) uniform Shared_Shader_Data {
        ivec2 mouse_old;
        ivec2 mouse_new;
        ivec2 mouse_stored;
        ivec2 window_size;

        int active_shape;
        int color;

        int stroke_scale;
    };

    vec3 colors_map[] = vec3[](
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1),
        vec3(0, 0, 0),
        vec3(1, 1, 0),
        vec3(0, 1, 1),
        vec3(1, 0, 1),
        vec3(1, 0.5, 0),
        vec3(1, 1, 1)
    );

    in vec2 vo_uv;

    void main() {
        vec2 dp = min(abs(vec2(mouse_stored) - vo_uv), abs(vec2(mouse_new) - vo_uv));
        if(min(dp.x, dp.y) > stroke_scale) {
            discard;
        }

        o_color = vec4(colors_map[color], 1);
    }
);

#endif