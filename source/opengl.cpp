typedef VoidFunc *OpenGLLoadFunc(char const *);
#define LOAD(def, name) name = (def*)load(#name); if (name == 0) return false;

INTERNAL bool load_opengl_functions(OpenGLLoadFunc *load) {
    LOAD(GL_GET_INTEGERV_FUNC, glGetIntegerv);
    LOAD(GL_ENABLE_FUNC, glEnable);
    LOAD(GL_DISABLE_FUNC, glDisable);
    LOAD(GL_GET_ERROR_FUNC, glGetError);
    LOAD(GL_CLEAR_COLOR_FUNC, glClearColor);
    LOAD(GL_CLEAR_FUNC, glClear);
    LOAD(GL_VIEWPORT_FUNC, glViewport);
    LOAD(GL_BLEND_FUNC_FUNC, glBlendFunc);
    LOAD(GL_GEN_VERTEX_ARRAYS_FUNC, glGenVertexArrays);
    LOAD(GL_DELETE_VERTEX_ARRAYS_FUNC, glDeleteVertexArrays);
    LOAD(GL_GEN_BUFFERS_FUNC, glGenBuffers);
    LOAD(GL_DELETE_BUFFERS_FUNC, glDeleteBuffers);
    LOAD(GL_BIND_VERTEX_ARRAY_FUNC, glBindVertexArray);
    LOAD(GL_BIND_BUFFER_FUNC, glBindBuffer);
    LOAD(GL_BIND_BUFFER_BASE_FUNC, glBindBufferBase);
    LOAD(GL_BUFFER_DATA_FUNC, glBufferData);
    LOAD(GL_BUFFER_SUB_DATA_FUNC, glBufferSubData);
    LOAD(GL_VERTEX_ATTRIB_POINTER_FUNC, glVertexAttribPointer);
    LOAD(GL_ENABLE_VERTEX_ATTRIB_ARRAY_FUNC, glEnableVertexAttribArray);
    LOAD(GL_USE_PROGRAM_FUNC, glUseProgram);
    LOAD(GL_UNIFORM_MATRIX_4_FV_FUNC, glUniformMatrix4fv);
    LOAD(GL_DRAW_ARRAYS_FUNC, glDrawArrays);

    LOAD(GL_CREATE_SHADER_FUNC, glCreateShader);
    LOAD(GL_SHADER_SOURCE_FUNC, glShaderSource);
    LOAD(GL_COMPILE_SHADER_FUNC, glCompileShader);
    LOAD(GL_CREATE_PROGRAM_FUNC, glCreateProgram);
    LOAD(GL_DELETE_PROGRAM_FUNC, glDeleteProgram);
    LOAD(GL_ATTACH_SHADER_FUNC, glAttachShader);
    LOAD(GL_LINK_PROGRAM_FUNC, glLinkProgram);
    LOAD(GL_DELETE_SHADER_FUNC, glDeleteShader);
    LOAD(GL_GET_UNIFORM_LOCATION_FUNC, glGetUniformLocation);
    LOAD(GL_GET_ATTRIB_LOCATION_FUNC, glGetAttribLocation);
    LOAD(GL_GET_UNIFORM_BLOCK_INDEX_FUNC, glGetUniformBlockIndex);
    LOAD(GL_UNIFORM_BLOCK_BINDING_FUNC, glUniformBlockBinding);
    LOAD(GL_GET_SHADER_IV_FUNC, glGetShaderiv);
    LOAD(GL_GET_SHADER_INFO_LOG_FUNC, glGetShaderInfoLog);
    LOAD(GL_GET_PROGRAM_IV_FUNC, glGetProgramiv);
    LOAD(GL_GET_PROGRAM_INFO_LOG_FUNC, glGetProgramInfoLog);
    LOAD(GL_GEN_TEXTURES_FUNC, glGenTextures);
    LOAD(GL_ACTIVE_TEXTURE_FUNC, glActiveTexture);
    LOAD(GL_BIND_TEXTURE_FUNC, glBindTexture);
    LOAD(GL_TEX_STORAGE_2D_FUNC, glTexStorage2D);
    LOAD(GL_COMPRESSED_TEX_SUB_IMAGE_2D_FUNC, glCompressedTexSubImage2D);
    LOAD(GL_TEX_IMAGE_2D_FUNC, glTexImage2D);
    LOAD(GL_TEX_PARAMETER_I_FUNC, glTexParameteri);
    LOAD(GL_GENERATE_MIPMAP_FUNC, glGenerateMipmap);

    LOAD(GL_DEPTH_FUNC_FUNC, glDepthFunc);
    LOAD(GL_SCISSOR_FUNC, glScissor);

    LOAD(GL_DEPTH_MASK_FUNC, glDepthMask);

    return true;
}

#undef LOAD

