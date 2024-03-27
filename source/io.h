#pragma once

#include "definitions.h"
#include "memory.h"


enum {
    LOG_NONE,
    LOG_NOTE,
    LOG_ERROR,
};
void log(s32 line, String file, u32 mode, char const *fmt, ...);
void log_to_file(struct PlatformFile *file);
void log_flush();

#define LOG(mode, fmt, ...) log(__LINE__, __FILE__, (mode), (fmt), __VA_ARGS__);

s64 convert_string_to_s64(u8 *buffer, s32 buffer_size);

s64 print(char const *fmt, ...);
s64 format(struct StringBuilder *builder, char const *fmt, ...);
s64 format(struct StringBuilder *builder, char const *fmt, va_list args);
s64 format(struct PlatformFile *file, char const *fmt, ...);
s64 format(struct PlatformFile *file, char const *fmt, va_list args);

s64 write(PlatformFile *file, void const *buffer, s64 size);
b32 write_byte(PlatformFile *file, u8 value);
b32 flush_write_buffer(PlatformFile *file);

enum {
    READ_ENTIRE_FILE_OK,
    READ_ENTIRE_FILE_NOT_FOUND,
    READ_ENTIRE_FILE_READ_ERROR,
};
String read_entire_file(String file, u32 *status = 0, Allocator alloc = default_allocator());


struct Path {
    DArray<u8> buffer;
};

void change_path(Path *path, String change);

