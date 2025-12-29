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

        vec2 P1A = P1 + vec2(-stroke_scale,  stroke_scale);
        vec2 P1B = P1 + vec2( stroke_scale,  stroke_scale);
        vec2 P1C = P1 + vec2( stroke_scale, -stroke_scale);
        vec2 P1D = P1 + vec2(-stroke_scale, -stroke_scale);

        vec2 vertices[] = vec2[](
            A, B, C,
            A, C, D,

            P2A, P2B, P2C,
            P2A, P2C, P2D,

            P1A, P1B, P1C,
            P1A, P1C, P1D
        );

        vec2 uvs[] = vec2[](
            vec2(0), vec2(0), vec2(0),
            vec2(0), vec2(0), vec2(0),

            vec2(-1, 1), vec2(1, 1), vec2(1, -1),
            vec2(-1, 1), vec2(1, -1), vec2(-1, -1),

            vec2(-1, 1), vec2(1, 1), vec2(1, -1),
            vec2(-1, 1), vec2(1, -1), vec2(-1, -1)
        );

        vec2 p;
        p = vertices[id];
        vo_uv = uvs[id];

        // else if(id < 6 + FAN_FRAGMENTS_COUNT * 3) {
        //     int sub_id = id % 3;
        //     int pie_segment = (id - 6) / 3;

        //     if(sub_id == 0) {
        //         float a2 = (sub_id + 1) * PI / FAN_FRAGMENTS_COUNT;
        //         p = P2 + rotx(-dR90, a2);
        //     } else if(sub_id == 1) {
        //         float a1 = sub_id * PI / FAN_FRAGMENTS_COUNT;
        //         p = P2 + rotx(-dR90, a1);
        //     } else {
        //         p = P2;
        //     }
        // }

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
