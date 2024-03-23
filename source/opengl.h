#pragma once

#include "opengl_definitions.h"


#if defined(OS_WINDOWS) && !defined(_WIN64)
#define OPENGL_CALL __stdcall
#else
#define OPENGL_CALL
#endif // defined(OD_WINDOWS) && !defined(_WIN64)

#ifdef OPENGL_DEFINE_FUNCTIONS
#define OPENGL_EXTERN
#else
#define OPENGL_EXTERN extern
#endif // OPENGL_DEFINE_FUNCTIONS



#define GL_FALSE			0
#define GL_TRUE				1
#define GL_UNSIGNED_BYTE		0x1401
#define GL_UNSIGNED_SHORT		0x1403
#define GL_UNSIGNED_INT			0x1405
#define GL_FLOAT			0x1406

#define GL_NO_ERROR			0
#define GL_LINES			0x0001
#define GL_LINE_STRIP			0x0003
#define GL_TRIANGLES			0x0004
#define GL_DEPTH_BUFFER_BIT		0x00000100
#define GL_COLOR_BUFFER_BIT		0x00004000
#define GL_ARRAY_BUFFER			0x8892
#define GL_STREAM_DRAW			0x88E0
#define GL_STATIC_DRAW			0x88E4
#define GL_DYNAMIC_DRAW			0x88E8
#define GL_FRAGMENT_SHADER		0x8B30
#define GL_GEOMETRY_SHADER		0x8DD9
#define GL_VERTEX_SHADER		0x8B31
#define GL_COMPILE_STATUS		0x8B81
#define GL_LINK_STATUS			0x8B82
#define GL_INFO_LOG_LENGTH		0x8B84
#define GL_TEXTURE_2D			0x0DE1
#define GL_TEXTURE_2D_ARRAY		0x8C1A
#define GL_RED				0x1903
#define GL_GREEN            0x1904
#define GL_BLUE             0x1905
#define GL_ALPHA			0x1906
#define GL_RGB              0x1907
#define GL_RGBA				0x1908
#define GL_R8				0x8229
#define GL_BLEND			0x0BE2
#define GL_SRC_ALPHA			0x0302
#define GL_ONE_MINUS_SRC_ALPHA		0x0303
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_WRAP_S		0x2802
#define GL_TEXTURE_WRAP_T		0x2803
#define GL_CLAMP_TO_EDGE		0x812F
#define GL_TEXTURE_MAG_FILTER		0x2800
#define GL_TEXTURE_MIN_FILTER		0x2801
#define GL_REPEAT                   0x2901
#define GL_NEAREST			0x2600
#define GL_LINEAR			0x2601
#define GL_CULL_FACE        0x0B44
#define GL_DEPTH_TEST			0x0B71
#define GL_SCISSOR_TEST			0x0C11
#define GL_MAX_TEXTURE_SIZE  0x0D33

#define GL_LEQUAL			0x0203

#define GL_RG               0x8227

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT		0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT	0x83F2

#define GL_UNIFORM_BUFFER   0x8A11

#define GL_TEXTURE0        0x84C0
#define GL_TEXTURE1        0x84C1
#define GL_TEXTURE2        0x84C2
#define GL_TEXTURE3        0x84C3
#define GL_TEXTURE4        0x84C4
#define GL_TEXTURE5        0x84C5
#define GL_TEXTURE6        0x84C6
#define GL_TEXTURE7        0x84C7
#define GL_TEXTURE8        0x84C8
#define GL_TEXTURE9        0x84C9
#define GL_TEXTURE10       0x84CA
#define GL_TEXTURE11       0x84CB
#define GL_TEXTURE12       0x84CC
#define GL_TEXTURE13       0x84CD
#define GL_TEXTURE14       0x84CE
#define GL_TEXTURE15       0x84CF
#define GL_TEXTURE16       0x84D0
#define GL_TEXTURE17       0x84D1
#define GL_TEXTURE18       0x84D2
#define GL_TEXTURE19       0x84D3
#define GL_TEXTURE20       0x84D4
#define GL_TEXTURE21       0x84D5
#define GL_TEXTURE22       0x84D6
#define GL_TEXTURE23       0x84D7
#define GL_TEXTURE24       0x84D8
#define GL_TEXTURE25       0x84D9
#define GL_TEXTURE26       0x84DA
#define GL_TEXTURE27       0x84DB
#define GL_TEXTURE28       0x84DC
#define GL_TEXTURE29       0x84DD
#define GL_TEXTURE30       0x84DE
#define GL_TEXTURE31       0x84DF

#define GL_INVALID_INDEX 0xFFFFFFFF

typedef void OPENGL_CALL GL_GET_INTEGERV_FUNC(GLenum, GLint *); OPENGL_EXTERN GL_GET_INTEGERV_FUNC *glGetIntegerv;
typedef void OPENGL_CALL GL_ENABLE_FUNC(GLenum); OPENGL_EXTERN GL_ENABLE_FUNC *glEnable;
typedef void OPENGL_CALL GL_DISABLE_FUNC(GLenum); OPENGL_EXTERN GL_DISABLE_FUNC *glDisable;
typedef GLenum OPENGL_CALL GL_GET_ERROR_FUNC(void); OPENGL_EXTERN GL_GET_ERROR_FUNC *glGetError;
typedef void OPENGL_CALL GL_CLEAR_COLOR_FUNC(GLfloat, GLfloat, GLfloat, GLfloat); OPENGL_EXTERN GL_CLEAR_COLOR_FUNC *glClearColor;
typedef void OPENGL_CALL GL_CLEAR_FUNC(GLbitfield); OPENGL_EXTERN GL_CLEAR_FUNC *glClear;
typedef void OPENGL_CALL GL_VIEWPORT_FUNC(GLint, GLint, GLsizei, GLsizei); OPENGL_EXTERN GL_VIEWPORT_FUNC *glViewport;
typedef void OPENGL_CALL GL_BLEND_FUNC_FUNC(GLenum, GLenum); OPENGL_EXTERN GL_BLEND_FUNC_FUNC *glBlendFunc;
typedef void OPENGL_CALL GL_GEN_VERTEX_ARRAYS_FUNC(GLsizei, GLuint*); OPENGL_EXTERN GL_GEN_VERTEX_ARRAYS_FUNC *glGenVertexArrays;
typedef void OPENGL_CALL GL_DELETE_VERTEX_ARRAYS_FUNC(GLsizei, GLuint const*); OPENGL_EXTERN GL_DELETE_VERTEX_ARRAYS_FUNC *glDeleteVertexArrays;
typedef void OPENGL_CALL GL_GEN_BUFFERS_FUNC(GLsizei, GLuint*); OPENGL_EXTERN GL_GEN_BUFFERS_FUNC *glGenBuffers;
typedef void OPENGL_CALL GL_DELETE_BUFFERS_FUNC(GLsizei, GLuint const*); OPENGL_EXTERN GL_DELETE_BUFFERS_FUNC *glDeleteBuffers;
typedef void OPENGL_CALL GL_BIND_VERTEX_ARRAY_FUNC(GLuint); OPENGL_EXTERN GL_BIND_VERTEX_ARRAY_FUNC *glBindVertexArray;
typedef void OPENGL_CALL GL_BIND_BUFFER_FUNC(GLenum, GLuint); OPENGL_EXTERN GL_BIND_BUFFER_FUNC *glBindBuffer;
typedef void OPENGL_CALL GL_BIND_BUFFER_BASE_FUNC(GLenum, GLuint, GLuint); OPENGL_EXTERN GL_BIND_BUFFER_BASE_FUNC *glBindBufferBase;
typedef void OPENGL_CALL GL_BUFFER_DATA_FUNC(GLuint, GLsizeiptr, void const*, GLenum); OPENGL_EXTERN GL_BUFFER_DATA_FUNC *glBufferData;
typedef void OPENGL_CALL GL_BUFFER_SUB_DATA_FUNC(GLenum, GLintptr, GLsizeiptr, void const*); OPENGL_EXTERN GL_BUFFER_SUB_DATA_FUNC *glBufferSubData;
typedef void OPENGL_CALL GL_VERTEX_ATTRIB_POINTER_FUNC(GLuint, GLuint, GLenum, GLboolean, GLsizei, void const*); OPENGL_EXTERN GL_VERTEX_ATTRIB_POINTER_FUNC *glVertexAttribPointer;
typedef void OPENGL_CALL GL_ENABLE_VERTEX_ATTRIB_ARRAY_FUNC(GLuint); OPENGL_EXTERN GL_ENABLE_VERTEX_ATTRIB_ARRAY_FUNC *glEnableVertexAttribArray;
typedef void OPENGL_CALL GL_USE_PROGRAM_FUNC(GLuint); OPENGL_EXTERN GL_USE_PROGRAM_FUNC *glUseProgram;
typedef void OPENGL_CALL GL_UNIFORM_MATRIX_4_FV_FUNC(GLint, GLsizei, GLboolean, GLfloat const*); OPENGL_EXTERN GL_UNIFORM_MATRIX_4_FV_FUNC *glUniformMatrix4fv;
typedef void OPENGL_CALL GL_DRAW_ARRAYS_FUNC(GLenum, GLint, GLsizei); OPENGL_EXTERN GL_DRAW_ARRAYS_FUNC *glDrawArrays;

typedef GLuint OPENGL_CALL GL_CREATE_SHADER_FUNC(GLenum); OPENGL_EXTERN GL_CREATE_SHADER_FUNC *glCreateShader;
typedef void OPENGL_CALL GL_SHADER_SOURCE_FUNC(GLuint, GLsizei, GLchar const**, GLint const*); OPENGL_EXTERN GL_SHADER_SOURCE_FUNC *glShaderSource;
typedef void OPENGL_CALL GL_COMPILE_SHADER_FUNC(GLuint); OPENGL_EXTERN GL_COMPILE_SHADER_FUNC *glCompileShader;
typedef GLuint OPENGL_CALL GL_CREATE_PROGRAM_FUNC(); OPENGL_EXTERN GL_CREATE_PROGRAM_FUNC *glCreateProgram;
typedef void OPENGL_CALL GL_DELETE_PROGRAM_FUNC(GLuint); OPENGL_EXTERN GL_DELETE_PROGRAM_FUNC *glDeleteProgram;
typedef void OPENGL_CALL GL_ATTACH_SHADER_FUNC(GLuint, GLuint); OPENGL_EXTERN GL_ATTACH_SHADER_FUNC *glAttachShader;
typedef void OPENGL_CALL GL_LINK_PROGRAM_FUNC(GLuint); OPENGL_EXTERN GL_LINK_PROGRAM_FUNC *glLinkProgram;
typedef void OPENGL_CALL GL_DELETE_SHADER_FUNC(GLuint); OPENGL_EXTERN GL_DELETE_SHADER_FUNC *glDeleteShader;
typedef GLint OPENGL_CALL GL_GET_UNIFORM_LOCATION_FUNC(GLuint, GLchar const*); OPENGL_EXTERN GL_GET_UNIFORM_LOCATION_FUNC *glGetUniformLocation;
typedef GLint OPENGL_CALL GL_GET_ATTRIB_LOCATION_FUNC(GLuint, GLchar const*); OPENGL_EXTERN GL_GET_ATTRIB_LOCATION_FUNC *glGetAttribLocation;
typedef GLuint OPENGL_CALL GL_GET_UNIFORM_BLOCK_INDEX_FUNC(GLuint, GLchar const*); OPENGL_EXTERN GL_GET_UNIFORM_BLOCK_INDEX_FUNC *glGetUniformBlockIndex;
typedef void OPENGL_CALL GL_UNIFORM_BLOCK_BINDING_FUNC(GLuint, GLuint, GLuint); OPENGL_EXTERN GL_UNIFORM_BLOCK_BINDING_FUNC *glUniformBlockBinding;
typedef void OPENGL_CALL GL_GET_SHADER_IV_FUNC(GLuint, GLenum, GLint*); OPENGL_EXTERN GL_GET_SHADER_IV_FUNC *glGetShaderiv;
typedef void OPENGL_CALL GL_GET_SHADER_INFO_LOG_FUNC(GLuint, GLsizei, GLsizei*, GLchar*); OPENGL_EXTERN GL_GET_SHADER_INFO_LOG_FUNC *glGetShaderInfoLog;
typedef void OPENGL_CALL GL_GET_PROGRAM_IV_FUNC(GLuint, GLenum, GLint*); OPENGL_EXTERN GL_GET_PROGRAM_IV_FUNC *glGetProgramiv;
typedef void OPENGL_CALL GL_GET_PROGRAM_INFO_LOG_FUNC(GLuint, GLsizei, GLsizei*, GLchar*); OPENGL_EXTERN GL_GET_PROGRAM_INFO_LOG_FUNC *glGetProgramInfoLog;
typedef void OPENGL_CALL GL_GEN_TEXTURES_FUNC(GLsizei, GLuint*); OPENGL_EXTERN GL_GEN_TEXTURES_FUNC *glGenTextures;
typedef void OPENGL_CALL GL_DELETE_TEXTURES_FUNC(GLsizei, GLuint*); OPENGL_EXTERN GL_DELETE_TEXTURES_FUNC *glDeleteTextures;
typedef void OPENGL_CALL GL_ACTIVE_TEXTURE_FUNC(GLenum); OPENGL_EXTERN GL_ACTIVE_TEXTURE_FUNC *glActiveTexture;
typedef void OPENGL_CALL GL_BIND_TEXTURE_FUNC(GLenum, GLuint); OPENGL_EXTERN GL_BIND_TEXTURE_FUNC *glBindTexture;
typedef void OPENGL_CALL GL_TEX_IMAGE_2D_FUNC(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, void const*); OPENGL_EXTERN GL_TEX_IMAGE_2D_FUNC *glTexImage2D;
typedef void OPENGL_CALL GL_COMPRESSED_TEX_SUB_IMAGE_2D_FUNC(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, void const*); OPENGL_EXTERN GL_COMPRESSED_TEX_SUB_IMAGE_2D_FUNC *glCompressedTexSubImage2D;
typedef void OPENGL_CALL GL_TEX_STORAGE_2D_FUNC(GLenum, GLsizei, GLenum, GLsizei, GLsizei); OPENGL_EXTERN GL_TEX_STORAGE_2D_FUNC *glTexStorage2D;
typedef void OPENGL_CALL GL_TEX_PARAMETER_I_FUNC(GLenum, GLenum, GLint); OPENGL_EXTERN GL_TEX_PARAMETER_I_FUNC *glTexParameteri;
typedef void OPENGL_CALL GL_GENERATE_MIPMAP_FUNC(GLenum); OPENGL_EXTERN GL_GENERATE_MIPMAP_FUNC *glGenerateMipmap;

typedef void OPENGL_CALL GL_DEPTH_FUNC_FUNC(GLenum); OPENGL_EXTERN GL_DEPTH_FUNC_FUNC *glDepthFunc;
typedef void OPENGL_CALL GL_SCISSOR_FUNC(GLint, GLint, GLsizei, GLsizei); OPENGL_EXTERN GL_SCISSOR_FUNC *glScissor;

typedef void OPENGL_CALL GL_DEPTH_MASK_FUNC(GLboolean); OPENGL_EXTERN GL_DEPTH_MASK_FUNC *glDepthMask;

#undef OPENGL_EXTERN

