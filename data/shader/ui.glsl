#if defined(VERTEX_SHADER_PART)

layout (location = 0) in vec3 pos;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 color_in;

out vec3 normal_vs;
out vec2 uv_vs;
out vec4 color_vs;

layout (std140) uniform Matrices {
    mat4 proj3D;
    mat4 proj2D;
    mat4 view;
};

void main() {
    gl_Position = proj2D * vec4(pos, 1.0);

    uv_vs = uv;
    color_vs = color_in;
}

#endif // defined(VERTEX_SHADER_PART)

#if defined(FRAGMENT_SHADER_PART)

in vec2 uv_vs;
in vec4 color_vs;

uniform sampler2D image;

out vec4 color;

void main() {
    color = vec4(color_vs.rgb, texture(image, uv_vs).r);
}

#endif // defined(FRAGMENT_SHADER_PART)

