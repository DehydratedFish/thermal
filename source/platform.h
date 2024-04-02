#pragma once

#include "memory.h"


#ifdef DEVELOPER
void fire_assert(char const *msg, char const *func, char const *file, int line);
#define assert(expr) (void)((expr) || (fire_assert(#expr, __func__, __FILE__, __LINE__),0))
#else
#define assert(expr)
#endif // DEVELOPER


typedef void RenderFunc(struct GameState*);


enum {
    PLATFORM_FILE_READ,
    PLATFORM_FILE_WRITE,
    PLATFORM_FILE_APPEND,
};
struct PlatformFile {
    void *handle;
    MemoryBuffer read_buffer;
    MemoryBuffer write_buffer;

    b32 open;
};


PlatformFile platform_create_file_handle(String filename, u32 mode);
void   platform_close_file_handle(PlatformFile *file);
s64    platform_file_size(PlatformFile *file);
String platform_read(PlatformFile *file, u64 offset, void *buffer, s64 size);
s64    platform_write(PlatformFile *file, void const *buffer, s64 size);
s64    platform_write(PlatformFile *file, u64 offset, void const *buffer, s64 size);

void platform_create_directory(String path);
void platform_delete_file_or_directory(String path);

b32 write_builder_to_file(struct StringBuilder *builder, String file);


b32 platform_is_relative_path(String path);

b32 platform_change_directory(String path);
String platform_get_current_directory();

Array<String> platform_directory_listing(String path);
void platform_destroy_directory_listing(Array<String> *listing);


struct PlatformConsole {
    PlatformFile *out;
};

extern PlatformConsole Console;


void *platform_allocate_raw_memory(s64 size);
void  platform_free_raw_memory(void *ptr);

bool platform_setup_window();
void platform_window_swap_buffers();
void platform_update(struct ApplicationState *state);

String platform_get_executable_path();
b32 platform_file_exists(String file);


String platform_file_selection_dialog(String path);


struct PlatformRingBuffer {
    void *memory;
    s32 size;
    s32 alloc;
    s32 end;
};


PlatformRingBuffer platform_create_ring_buffer(s32 size);

// Returns the next writable area in the buffer and advances .end accordingly.
// If the requested size is bigger than .alloc the returned range will be smaller than size
// and the write needs to be split.
//
// If offset is specified then the write will begin before the end of the buffer,
// overriding the contents.
String platform_writable_range(PlatformRingBuffer *ring, s32 size, s32 offset = 0);

// Same as above but the contents after the offset are preserved and moved backwards.
// The oldest bytes will be overridden.
String platform_writable_range_inserted(PlatformRingBuffer *ring, s32 size, s32 offset = 0);


enum {
    PLATFORM_THREAD_SUSPENDED,
    PLATFORM_THREAD_RUNNING,
    PLATFORM_THREAD_ERROR,
};
typedef s32 PlatformThreadFunc(void*);
struct PlatformThread {
    void *platform_data;

    PlatformThreadFunc *func;
    void *user_data;

    s32 result;
};

PlatformThread *platform_create_thread(PlatformThreadFunc *func, void *user_data);
void platform_destroy_thread(PlatformThread *thread);
void platform_join_thread(PlatformThread thread);


// NOTE: I want to replace the win32 nonesense with a hand tailored include.
struct PlatformExecutionContext {
    void *read_pipe;

    void *thread_handle;
    void *process_handle;

    u32 started_successfully;
};

PlatformExecutionContext platform_execute(String command);
b32 platform_finished_execution(PlatformExecutionContext *pec);
u32 platform_input_available(PlatformExecutionContext *pec);
s32 platform_read(PlatformExecutionContext *pec, void *buffer, s32 size);

s32 application_main(Array<String> args);

