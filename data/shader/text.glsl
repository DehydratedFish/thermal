#if defined(VERTEX_SHADER_PART)

layout (location = 0) in vec3 pos;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 fg_in;
layout (location = 4) in vec4 bg_in;

out vec2 uv_vs;
out vec4 fg_vs;
out vec4 bg_vs;

layout (std140) uniform Matrices {
    mat4 proj3D;
    mat4 proj2D;
    mat4 view;
};

void main() {
    gl_Position = proj2D * vec4(pos, 1.0);

    uv_vs = uv;
    fg_vs = fg_in;
    bg_vs = bg_in;
}

#endif // defined(VERTEX_SHADER_PART)

#if defined(FRAGMENT_SHADER_PART)

in vec2 uv_vs;
in vec4 fg_vs;
in vec4 bg_vs;

uniform sampler2D image;

out vec4 color;

void main() {
    if (bg_vs.a == 0.0) {
        color = vec4(fg_vs.rgb, texture(image, uv_vs).r);
    } else {
        color = mix(bg_vs, fg_vs, texture(image, uv_vs).r);
    }
}

#endif // defined(FRAGMENT_SHADER_PART)

