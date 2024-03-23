#pragma once

#include "definitions.h"


typedef char GLchar;
typedef u8 GLboolean;
typedef s32 GLint;
typedef u32 GLuint;
typedef float GLfloat;
typedef s32 GLsizei;
typedef u32 GLenum;
typedef u32 GLbitfield;

#if defined(OS_WINDOWS) && !defined(_WIN64)
typedef s32 GLsizeiptr;
typedef s32 GLintptr;
#else
typedef s64 GLsizeiptr;
typedef s64 GLintptr;
#endif


struct GPUShader {
    GLuint id;
};

struct GPUBuffer {
    GLuint vao;
    GLuint vbo;

    s32 vertex_size;
};

struct GPUTexture {
    GLuint id;
    GLenum format;
    V2i size;
};

struct RendererBackend {
    GLuint matrix_buffer;
};

u32 const ShaderPositionLocation = 0;
u32 const ShaderNormalLocation   = 1;
u32 const ShaderUVLocation       = 2;
u32 const ShaderColorLocation    = 3;

u32 const ShaderForegroundColorLocation = 3;
u32 const ShaderBackgroundColorLocation = 4;

u32 const ShaderMatrixBlockIndex   = 0;
u32 const ShaderMaterialBlockIndex = 1;


