INTERNAL GLuint load_shader(String file);

void set_background(Renderer *renderer, Color color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void clear_background(Renderer *renderer) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void update_render_area(Renderer *renderer, V2 size) {
    glViewport(0, 0, (GLuint)size.width, (GLuint)size.height);
}

void create_gpu_buffer(GPUBuffer *buffer, u32 vertex_size, void *vertex_buffer, u32 buffer_size, Array<VertexBinding> bindings) {
    if (buffer->vao != 0) glDeleteVertexArrays(1, &buffer->vao);
    if (buffer->vbo != 0) glDeleteBuffers(1, &buffer->vbo);

    buffer->vertex_size = vertex_size;

    glGenVertexArrays(1, &buffer->vao);
    glBindVertexArray(buffer->vao);

    glGenBuffers(1, &buffer->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer_size * vertex_size, vertex_buffer, GL_STATIC_DRAW);

    FOR (bindings, binding) {
        void *offset = store_as_pointer(binding->offset);

        if (binding->kind == VERTEX_COMPONENT_V2) {
            glVertexAttribPointer(binding->shader_binding, 2, GL_FLOAT, GL_FALSE, vertex_size, offset);
        } else if (binding->kind == VERTEX_COMPONENT_V3) {
            glVertexAttribPointer(binding->shader_binding, 3, GL_FLOAT, GL_FALSE, vertex_size, offset);
        } else if (binding->kind == VERTEX_COMPONENT_V4) {
            glVertexAttribPointer(binding->shader_binding, 4, GL_FLOAT, GL_FALSE, vertex_size, offset);
        } else if (binding->kind == VERTEX_COMPONENT_PACKED_COLOR) {
            glVertexAttribPointer(binding->shader_binding, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertex_size, offset);
        } else {
            die("Malformed VertexComponentKind.");
        }

        glEnableVertexAttribArray(binding->shader_binding);
    }
}

void update_gpu_buffer(GPUBuffer *buffer, void *vertex_buffer, u32 buffer_size) {
    if (buffer->vbo == 0 || buffer->vertex_size == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);
    glBufferData(GL_ARRAY_BUFFER, buffer_size * buffer->vertex_size, vertex_buffer, GL_STATIC_DRAW);
}

void create_gpu_texture(GPUTexture *tex, V2i dimensions, u32 channels, String pixel_data, GPUTextureFlags flags) {
    assert(channels <= 4);

    u32 format;
    if      (channels == 1) format = GL_RED;
    else if (channels == 2) format = GL_RG;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    tex->format = format;
    tex->size   = dimensions;

    u32 wrap_param = flags & GPU_TEXTURE_CLAMP ? GL_CLAMP_TO_EDGE : GL_REPEAT;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_param);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, dimensions.width, dimensions.height, 0, format, GL_UNSIGNED_BYTE, pixel_data.data);
}

void update_gpu_texture(GPUTexture *tex, String pixel_data) {
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glTexImage2D(GL_TEXTURE_2D, 0, tex->format, tex->size.width, tex->size.height, 0, tex->format, GL_UNSIGNED_BYTE, pixel_data.data);
}

void create_gpu_shader(GPUShader *shader, String name) {
    shader->id = load_shader(name);
}

void render(Renderer *renderer, RenderTask *task) {
    glUseProgram(task->shader->id);
    glBindVertexArray(task->buffer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, task->buffer->vbo);

    if (task->texture->id) {
        use_texture(renderer, task->texture);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glDrawArrays(GL_TRIANGLES, task->start, task->count);
}

void init_renderer(Renderer *renderer) {
    INIT_STRUCT(renderer);

    // NOTE: set the default texture to white
    u32 white = PACK_RGB(255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);

    glGenBuffers(1, &renderer->backend.matrix_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->backend.matrix_buffer);
    glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(M4), 0, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, ShaderMatrixBlockIndex, renderer->backend.matrix_buffer);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void destroy_renderer(Renderer *renderer) {
    glDeleteBuffers(1, &renderer->backend.matrix_buffer);

    destroy(renderer->tasks);
}

void use_texture(Renderer *renderer, GPUTexture *texture) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id);
}

u32 const Proj3DOffset = 0;
u32 const Proj2DOffset = Proj3DOffset + sizeof(M4);
u32 const CameraOffset = Proj2DOffset + sizeof(M4);

INTERNAL void print_shader_debug_info(GLuint shader) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        if (length > 1) {
            char *log = ALLOC(default_allocator(), char, length);

            glGetShaderInfoLog(shader, length, 0, log);
            print("Shader compilation error:\n%s\n", log);

            DEALLOC(default_allocator(), log, length);
        }
    }
}

INTERNAL void print_program_debug_info(GLuint program) {
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        if (length > 1) {
            char *log = ALLOC(default_allocator(), char, length);

            glGetProgramInfoLog(program, length, 0, log);
            print("Program linking error:\n%s\n", log);

            DEALLOC(default_allocator(), log, length);
        }
    }
}

INTERNAL GLchar const *VertexShaderPart   = "#version 330 core\n#define VERTEX_SHADER_PART\n";
INTERNAL GLchar const *FragmentShaderPart = "#version 330 core\n#define FRAGMENT_SHADER_PART\n";
INTERNAL GLchar const *GeometryShaderPart = "#version 330 core\n#define GEOMETRY_SHADER_PART\n";

INTERNAL String ShaderDirectory = "data/shader";

INTERNAL GLuint load_shader(String file) {
    String path = t_format("%S/%S", ShaderDirectory, file);

    u32 status = 0;
    String source = read_entire_file(path, &status);
    DEFER(destroy_string(&source));

    if (status) {
        if (status == READ_ENTIRE_FILE_NOT_FOUND) {
            LOG(LOG_ERROR, "Could load shader %S\n", path);
        }
        if (status == READ_ENTIRE_FILE_READ_ERROR) {
            LOG(LOG_ERROR, "Could load but not read shader %S\n", path);
        }

        return 0;
    }

    GLuint program = glCreateProgram();

    GLchar const *sources[2];
    GLint lengths[2];

    sources[1] = (GLchar*)source.data;
    lengths[1] = source.size;

    sources[0] = VertexShaderPart;
    lengths[0] = c_string_length(VertexShaderPart);

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 2, sources, lengths);
    glCompileShader(vertex);

#if DEVELOPER
    print_shader_debug_info(vertex);
#endif

    glAttachShader(program, vertex);

    sources[0] = FragmentShaderPart;
    lengths[0] = c_string_length(FragmentShaderPart);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 2, sources, lengths);
    glCompileShader(fragment);

#if DEVELOPER
    print_shader_debug_info(fragment);
#endif

    glAttachShader(program, fragment);

    glLinkProgram(program);

#if DEVELOPER
    print_program_debug_info(program);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLuint shader_binding;
    shader_binding = glGetUniformBlockIndex(program, "Matrices");
    if (shader_binding != GL_INVALID_INDEX) {
        glUniformBlockBinding(program, shader_binding, ShaderMatrixBlockIndex);
    }

    shader_binding = glGetUniformBlockIndex(program, "Material");
    if (shader_binding != GL_INVALID_INDEX) {
        glUniformBlockBinding(program, shader_binding, ShaderMaterialBlockIndex);
    }

    return program;
}
void update_3D_projection(Renderer *renderer, M4 matrix) {
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->backend.matrix_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, Proj3DOffset, sizeof(M4), matrix.value);
}
void update_2D_projection(Renderer *renderer, M4 matrix) {
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->backend.matrix_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, Proj2DOffset, sizeof(M4), matrix.value);
}
void update_camera_matrix(Renderer *renderer, M4 matrix) {
    glBindBuffer(GL_UNIFORM_BUFFER, renderer->backend.matrix_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, CameraOffset, sizeof(M4), matrix.value);
}

