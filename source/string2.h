#pragma once

#include "definitions.h"
#include "memory.h"
#include "utf.h"


struct CharResult {
    u8  character;
    b32 empty;
};

inline CharResult get(String *str) {
    if (str->size == 0) return {0, true};

    u8 result = str->data[0];
    str->data += 1;
    str->size -= 1;

    return {result};
}

inline CharResult peek(String *str) {
    if (str->size == 0) return {0, true};

    return {str->data[0]};
}

inline u8 last(String str) {
    if (str.size) return str[str.size - 1];

    return 0;
}

struct GetLineResult {
    String line;
    b32 empty;
};

inline GetLineResult get_text_line(String *str) {
    if (str == 0 || str->size == 0) return {{}, true};

    String line = *str;
    line.size = 0;

    for (CharResult c = get(str); !c.empty; c = get(str)) {
        if (c.character == '\r') {
            c = peek(str);
            if (c.character == '\n') {
                str->data += 1;
                str->size -= 1;
            }

            break;
        }
        if (c.character == '\n') break;

        line.size += 1;
    }

    return {line, false};
}

inline bool equal(String lhs, String rhs) {
    if (lhs.size == rhs.size) {
        for (s64 i = 0; i < lhs.size; i += 1) {
            if (lhs[i] != rhs[i]) return false;
        }

        return true;
    }

    return false;
}

inline u8 lower_char(u8 c) {
    if (c >= 'A' && c <= 'Z') {
        c |= 32;
    }

    return c;
}

inline bool equal_insensitive(String lhs, String rhs) {
    if (lhs.size == rhs.size) {
        for (s64 i = 0; i < lhs.size; i += 1) {
            if (lower_char(lhs[i]) != lower_char(rhs[i])) return false;
        }

        return true;
    }

    return false;
}

inline bool contains(String str, String search) {
    for (s64 i = 0; i < str.size; i += 1) {
        if (i + search.size > str.size) return false;

        String tmp = {str.data + i, search.size};
        if (equal(tmp, search)) return true;
    }

    return false;
}

inline String shrink_front(String str, s64 amount = 1) {
#ifdef BOUNDS_CHECKING
    if (str.size < amount) die("String is too small to shrink.");
#endif
    str.data += amount;
    str.size -= amount;

    return str;
}

inline String shrink_back(String str, s64 amount = 1) {
#ifdef BOUNDS_CHECKING
    if (str.size < amount) die("String is too small to shrink.");
#endif
    str.size -= amount;

    return str;
}

inline String shrink(String str, s64 amount = 1) {
#ifdef BOUNDS_CHECKING
    if (str.size < amount * 2) die("String is too small to shrink.");
#endif
    return shrink_back(shrink_front(str, amount), amount);
}

inline String trim(String str) {
    s64 num_whitespaces = 0;

    for (s64 i = 0; i < str.size; i += 1) {
        if (str[i] > ' ') break;

        num_whitespaces += 1;
    }
    str.data += num_whitespaces;
    str.size -= num_whitespaces;

    num_whitespaces = 0;
    for (s64 i = str.size; i > 0; i -= 1) {
        if (str[i - 1] > ' ') break;

        num_whitespaces += 1;
    }
    str.size -= num_whitespaces;

    return str;
}

inline String path_without_filename(String path) {
    for (s64 i = path.size; i > 0; i -= 1) {
        if (path[i - 1] == '/' || path[i - 1] == '\\') {
            return {path.data, i - 1};
        }
    }

    return path;
}

inline bool operator==(String lhs, String rhs) {
    return equal(lhs, rhs);
}

inline bool operator!=(String lhs, String rhs) {
    return !(equal(lhs, rhs));
}

inline Ordering compare(String lhs, String rhs) {
	for (s32 i = 0; i != lhs.size && i != rhs.size; i += 1) {
		if (lhs[i] < rhs[i]) return CMP_LESSER;
		if (lhs[i] > rhs[i]) return CMP_GREATER;
	}

	s32 order = lhs.size - rhs.size;
	if (order < 0) return CMP_LESSER;
	else if (order > 0) return CMP_GREATER;
	else return CMP_EQUAL;
}

inline b32 starts_with(String str, String begin) {
    if (str.size < begin.size) return false;

    str.size = begin.size;
    return str == begin;
}

inline bool ends_with(String str, String search) {
    if (str.size < search.size) return false;

    str.data += str.size - search.size;
    str.size  = search.size;

    return str == search;
}

inline s64 find_first(String str, u32 c) {
    s64 result = -1;

    for (UTF8Iterator it = make_utf8_it(str); it.valid; next(&it)) {
        if (it.cp == c) {
            result = it.index;
            break;
        }
    }

    return result;
}

inline Array<String> split(String text, u8 c, Allocator alloc = temporary_allocator()) {
    DArray<String> lines = {};
    lines.allocator = alloc;

    String *line = append(lines, {text.data, 0});

    for (s64 i = 0; i < text.size; i += 1) {
        if (text[i] == c) {
            line = append(lines, {text.data + i + 1, 0});
        } else {
            line->size += 1;
        }
    }

    return lines;
}

inline Array<String> split_by_line_ending(String text, Allocator alloc = temporary_allocator()) {
    DArray<String> lines = {};
    lines.allocator = alloc;

    String *line = append(lines, {text.data, 0});

    u32 const multi_char_line_end = '\n' + '\r';
    for (s64 i = 0; i < text.size; i += 1) {
        if (text[i] == '\n' || text[i] == '\r') {
            if (i + 1 < text.size && text[i] + text[i + 1] == multi_char_line_end) {
                i += 1;
            }

            line = append(lines, {text.data + i + 1, 0});
        } else {
            line->size += 1;
        }
    }

    return lines;
}

struct SplitResult {
    String first;
    String second;
};
inline SplitResult split_at(String text, u8 c) { // TODO: utf awareness
    SplitResult result = {};

    s64 pos = find_first(text, c);
    if (pos == -1) return result;

    result.first  = trim({text.data, pos});
    pos += 1;
    result.second = trim({text.data + pos, text.size - pos});

    return result;
}

inline String sub_string(String buffer, s64 offset, s64 size) {
#ifdef BOUNDS_CHECKING
    if (buffer.size < offset + size) die("String read out of bounds.");
#endif
    String result = {buffer.data + offset, size};

    return result;
}

inline s32 write_le_u32(String buffer, s64 offset, u32 value) {
#ifdef BOUNDS_CHECKING
    if (buffer.size < offset + sizeof(u32)) die("String write out of bounds.");
#endif

    u8 *data = buffer.data + offset;
    data[0] = value & 0x000000FF;
    data[1] = (value & 0x0000FF00) >> 8;
    data[2] = (value & 0x00FF0000) >> 16;
    data[3] = (value & 0xFF000000) >> 24;

    return 4;
}

inline s32 write_le_s64(String buffer, s64 offset, s64 value) {
#ifdef BOUNDS_CHECKING
    if (buffer.size < offset + sizeof(s64)) die("String write out of bounds.");
#endif

    u8 *data = buffer.data + offset;
    data[0] = value & 0x000000FF;
    data[1] = (value & 0x0000FF00) >> 8;
    data[2] = (value & 0x00FF0000) >> 16;
    data[3] = (value & 0xFF000000) >> 24;
    data[4] = (value & 0xFF000000) >> 32;
    data[5] = (value & 0xFF000000) >> 40;
    data[6] = (value & 0xFF000000) >> 48;
    data[7] = (value & 0xFF000000) >> 56;

    return 8;
}

inline s32 write_string_with_size(String buffer, s64 offset, String data) {
    s32 size = data.size;

#ifdef BOUNDS_CHECKING
    if (buffer.size < offset + data.size + sizeof(size)) die("String write out of bounds.");
#endif

    write_le_u32(buffer, offset, size);
    copy_memory(buffer.data + offset + sizeof(size), data.data, data.size);

    return sizeof(size) + data.size;
}

inline s32 write_raw(String buffer, s64 offset, String data) {
#ifdef BOUNDS_CHECKING
    if (buffer.size < offset + data.size) die("String write out of bounds.");
#endif

    copy_memory(buffer.data + offset, data.data, data.size);

    return data.size;
}

// NOTE: simple djb2 hash
inline u32 string_hash(String string) {
	u32 hash = 5381;

	for (s32 i = 0; i < string.size; i += 1) {
		hash = hash * 33 ^ string[i];
	}

	return hash;
}

String format(char const *fmt, ...);
String t_format(char const *fmt, ...);

inline String allocate_string(s64 size, Allocator alloc = default_allocator()) {
    String result = {};
    result.data = ALLOC(alloc, u8, size);
    result.size = size;

    return result;
}

inline String allocate_temp_string(s64 size) {
    String result = {};
    result.data = (u8*)allocate(temporary_allocator(), size);
    result.size = size;

    return result;
}

inline String allocate_string(String str, Allocator alloc = default_allocator()) {
    String result = allocate_string(str.size, alloc);

    copy_memory(result.data, str.data, str.size);

    return result;
}

inline String allocate_temp_string(String str) {
    String result = allocate_temp_string(str.size);

    copy_memory(result.data, str.data, str.size);

    return result;
}

inline char const *temporary_c_string(String str) {
    String result = allocate_temp_string(str.size + 1);
    copy_memory(result.data, str.data, str.size);
    result.data[result.size - 1] = '\0';

    return (char const*)result.data;
}

inline void destroy_string(String *str, Allocator alloc = default_allocator()) {
    DEALLOC(alloc, str->data, str->size);

    INIT_STRUCT(str);
}


#ifndef STRING_BUILDER_BLOCK_SIZE
#define STRING_BUILDER_BLOCK_SIZE KILOBYTES(4)
#endif

struct StringBuilderBlock {
    StringBuilderBlock *next;

    u8 buffer[STRING_BUILDER_BLOCK_SIZE];
    s64 used;
};
struct StringBuilder {
    Allocator allocator;

    StringBuilderBlock  first;
    StringBuilderBlock *current;

    s64 total_size;
};

inline void reset(StringBuilder *builder) {
    StringBuilderBlock *block = &builder->first;

    while (block) {
        block->used = 0;
        block = block->next;
    }

    builder->total_size = 0;
}

inline void append(StringBuilder *builder, u8 c, Allocator alloc = default_allocator()) {
    if (builder->current == 0) builder->current = &builder->first;
    if (builder->allocator.allocate == 0) builder->allocator = alloc;

    if (builder->current->used + 1 > STRING_BUILDER_BLOCK_SIZE) {
        if (builder->current->next == 0) {
            builder->current->next = ALLOC(builder->allocator, StringBuilderBlock, 1);
        } else {
            // NOTE: buffer not initialized to zero for speed
            builder->current->next->used = 0;
        }
        builder->current = builder->current->next;
    }

    builder->current->buffer[builder->current->used] = c;
    builder->current->used += 1;
    builder->total_size    += 1;
}

inline void append(StringBuilder *builder, String str, Allocator alloc = default_allocator()) {
    if (builder->current == 0) builder->current = &builder->first;
    if (builder->allocator.allocate == 0) builder->allocator = alloc;

    s64 space = STRING_BUILDER_BLOCK_SIZE - builder->current->used;
    while (space < str.size) {
        copy_memory(builder->current->buffer + builder->current->used, str.data, space);
        builder->current->used += space;
        builder->total_size    += space;
        str = shrink_front(str, space);

        if (builder->current->next == 0) {
            builder->current->next = ALLOC(builder->allocator, StringBuilderBlock, 1);
        } else {
            builder->current->next->used = 0;
        }
        builder->current = builder->current->next;

        space = STRING_BUILDER_BLOCK_SIZE;
    }

    copy_memory(builder->current->buffer + builder->current->used, str.data, str.size);
    builder->current->used += str.size;
    builder->total_size    += str.size;
}

inline void append_raw(StringBuilder *builder, void *buffer, s64 size, Allocator alloc = default_allocator()) {
    String str = {(u8*)buffer, size};
    append(builder, str, alloc);
}
#define APPEND_RAW(builder, value) append_raw((builder), (void*)&(value), sizeof(value))

inline void destroy(StringBuilder *builder) {
    StringBuilderBlock *next = builder->first.next;

    while (next) {
        StringBuilderBlock *tmp = next->next;
        DEALLOC(builder->allocator, next, 1);

        next = tmp;
    }
}

inline String to_allocated_string(StringBuilder *builder, Allocator alloc = default_allocator()) {
    String result = allocate_string(builder->total_size, alloc);

    s64 size = 0;
    StringBuilderBlock *block = &builder->first;
    while (block) {
        copy_memory(result.data + size, block->buffer, block->used);
        size += block->used;

        block = block->next;
    }

    return result;
}

inline String temp_string(StringBuilder *builder) {
    assert(builder->total_size < STRING_BUILDER_BLOCK_SIZE);

    return {builder->first.buffer, builder->first.used};
}

