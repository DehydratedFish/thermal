#pragma once

#include "definitions.h"
#include "stb_truetype.h"

#include "hash_table.h"


// TODO: should suffice for testing but a proper hash function is probably needed
inline u32 glyph_hash(u32 cp) {
    cp = ((cp >> 16) ^ cp) * 0x45d9f3b;
    cp = ((cp >> 16) ^ cp) * 0x45d9f3b;
    cp =  (cp >> 16) ^ cp;

    return cp;
}

struct GlyphInfo {
    r32 advance;

    r32 x0, y0, x1, y1;
    r32 u0, v0, u1, v1;
};

// TODO: use the maximum texture size of the GPU
#define FONT_ATLAS_DIMENSION 2048
r32 const FONT_ATLAS_RATIO = 1.0f / FONT_ATLAS_DIMENSION;
struct Font {
    String data;
    stbtt_fontinfo stb_info;

    r32 scale;
    r32 ascent;
    r32 line_advance;

    r32 tile_width;
    r32 tile_height;

    s32 glyphs_per_line;
    s32 tile_max;
    s32 tile_count;

    b32 is_dirty;

    String atlas;
    HashTable<u32, GlyphInfo, u32, glyph_hash> glyph_table;
};


GlyphInfo *get_glyph(Font *font, u32 cp);
void init(Font *font, r32 height, String font_data);
void destroy(Font *font);

r32 text_width(Font *font, String text);
V2 font_text_metrics(Font *font, String text);




enum ConsoleFontKind {
    CONSOLE_FONT_REGULAR,
    CONSOLE_FONT_BOLD,
    CONSOLE_FONT_LIGHT,
    CONSOLE_FONT_ITALIC,
    CONSOLE_FONT_BOLD_ITALIC,

    CONSOLE_FONT_KIND_COUNT,
};

struct STBFont {
    String data;
    stbtt_fontinfo info;

    r32 scale;

    r32 ascent;
    r32 descent;
};

struct ConsoleGlyphInfo {
    V2i offset_in_atlas[CONSOLE_FONT_KIND_COUNT];
    b32 loaded[CONSOLE_FONT_KIND_COUNT];
};

struct ConsoleFont {
    Allocator allocator;

    STBFont font_data[CONSOLE_FONT_KIND_COUNT];

    s32 glyph_width;
    s32 glyph_height;

    b32 is_dirty;

    s32 glyphs_per_line;
    s32 max_glyphs;

    V2i next_free_glyph;

    String atlas;
    HashTable<u32, ConsoleGlyphInfo, u32, glyph_hash> glyph_table;
};

void init(ConsoleFont *font, r32 height, String font_dir, String font_family, Allocator alloc = default_allocator());

ConsoleGlyphInfo *get_glyph(ConsoleFont *font, u32 kind, u32 cp);

