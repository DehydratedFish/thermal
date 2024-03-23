#pragma once


// TODO: Detect what graphics library to use if nothing is set
#define USE_OPENGL

#ifdef USE_OPENGL
#include "opengl_definitions.h"
#endif


Color const TextForeground   = {1.0f, 1.0f, 1.0f, 1.0f};
Color const ButtonBackground = {0.35f, 0.35f, 0.35f, 1.0f};
Color const ButtonHovered    = {0.45f, 0.45f, 0.45f, 1.0f};
Color const ButtonClicked    = {0.65f, 0.65f, 0.65f, 1.0f};
Color const ButtonInactiveBackground = {0.0f, 0.0f, 0.0f, 1.0f};


struct RenderTask {
    GPUShader  *shader;
    GPUBuffer  *buffer;
    GPUTexture *texture;

    u32 start;
    u32 count;
};

struct Renderer {
    RendererBackend backend;

    DArray<RenderTask> tasks;
};

enum VertexComponentKind {
    VERTEX_COMPONENT_V2,
    VERTEX_COMPONENT_V3,
    VERTEX_COMPONENT_V4,
    VERTEX_COMPONENT_PACKED_COLOR,
};
struct VertexBinding {
    VertexComponentKind kind;
    u32 shader_binding;
    u32 offset;
};

enum ImageKind {
    IMAGE_RGBA,
};

enum GPUTextureFlags {
    GPU_TEXTURE_FLAGS_NONE = 0x00,
    GPU_TEXTURE_CLAMP      = 0x01,
};

void init_renderer(Renderer *renderer);
void destroy_renderer(Renderer *renderer);

void set_background(Renderer *renderer, Color color);
void clear_background(Renderer *renderer);

void update_render_area(Renderer *renderer, V2 size);

M4 ortho(r32 width, r32 height);
M4 perspective_lh(r32 fov, r32 aspect, r32 near, r32 far);
M4 perspective_rh(r32 fov, r32 aspect, r32 near, r32 far);

void update_3D_projection(Renderer *renderer, M4 matrix);
void update_2D_projection(Renderer *renderer, M4 matrix);
void update_camera_matrix(Renderer *renderer, M4 matrix);

void create_gpu_buffer(GPUBuffer *buffer, u32 vertex_size, void *vertex_buffer, u32 buffer_size, Array<VertexBinding> bindings);
void update_gpu_buffer(GPUBuffer *buffer, void *vertex_buffer, u32 buffer_size);

void create_gpu_texture(GPUTexture *tex, V2i dimensions, u32 channels, String pixel_data, GPUTextureFlags flags = GPU_TEXTURE_FLAGS_NONE);
void update_gpu_texture(GPUTexture *tex, String pixel_data);

void use_texture(Renderer *renderer, GPUTexture *texture);

void create_gpu_shader(GPUShader *shader, String name);

void render(Renderer *renderer, RenderTask *task);

