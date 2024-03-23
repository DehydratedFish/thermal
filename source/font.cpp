#include "font.h"

#include "string2.h"


#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"


INTERNAL GlyphInfo *font_add_glyph(Font *font, u32 cp, u32 mapping = 0) {
    if (font->tile_count == font->tile_max) {
        // TODO: path for when the cache is full
    } else {
        r32 tile_x = (font->tile_count % font->glyphs_per_line) * font->tile_width;
        r32 tile_y = (u32)(font->tile_count / font->glyphs_per_line) * font->tile_height;

        int glyph = stbtt_FindGlyphIndex(&font->stb_info, cp);
        if (glyph == 0) return get_glyph(font, '?'); // TODO: proper replacement character

        int x0, x1, y0, y1;
        stbtt_GetGlyphBitmapBox(&font->stb_info, glyph, font->scale, font->scale, &x0, &y0, &x1, &y1);

        s32 height = (y1 - y0);
        s32 width = (x1 - x0);

        int advance_width, lsb;
        stbtt_GetGlyphHMetrics(&font->stb_info, glyph, &advance_width, &lsb);

        r32 lsb_tmp = lsb * font->scale;

        s32 y_adjust = (s32)font->ascent + y0;
        s32 pos = (s32)(((tile_y + y_adjust) * FONT_ATLAS_DIMENSION) + tile_x + lsb_tmp);
        stbtt_MakeGlyphBitmap(&font->stb_info, &font->atlas[pos], width, height, FONT_ATLAS_DIMENSION, font->scale, font->scale, glyph);

        GlyphInfo info;

        info.advance = advance_width * font->scale;
        info.x0 = lsb_tmp;
        info.y0 = (r32)y0;
        info.x1 = info.x0 + width;
        info.y1 = info.y0 + height;
        info.u0 = tile_x / FONT_ATLAS_DIMENSION;
        info.v0 = tile_y / FONT_ATLAS_DIMENSION;
        info.u1 = (tile_x + font->tile_width)  / FONT_ATLAS_DIMENSION;
        info.v1 = (tile_y + font->tile_height) / FONT_ATLAS_DIMENSION;

        font->tile_count += 1;

        font->is_dirty = true;

        if (mapping != 0) cp = mapping;
        return insert(&font->glyph_table, cp, info);
    }
    return 0;
}

void init(Font *font, r32 height, String font_data) {
    font->data = font_data;
    stbtt_InitFont(&font->stb_info, font_data.data, stbtt_GetFontOffsetForIndex(font_data.data, 0));
    font->scale = stbtt_ScaleForPixelHeight(&font->stb_info, height);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font->stb_info, &ascent, &descent, &line_gap);

    font->ascent = (ascent * font->scale);
    r32 descent_tmp  = (descent * font->scale);
    r32 line_gap_tmp = (line_gap * font->scale);

    font->line_advance = font->ascent - descent_tmp + line_gap_tmp;

    int x0, y0, x1, y1;
    stbtt_GetFontBoundingBox(&font->stb_info, &x0, &y0, &x1, &y1);

    int advance_width, lsb;
    stbtt_GetCodepointHMetrics(&font->stb_info, 'M', &advance_width, &lsb);

    // TODO: I really don't know if this is correct... but the font bounding box is too large and looks ugly.
    font->tile_width  = ceil(advance_width  * font->scale);
    font->tile_height = ceil(font->ascent - descent_tmp);

    font->glyphs_per_line = (s32)(FONT_ATLAS_DIMENSION / font->tile_width);
    font->tile_max = font->glyphs_per_line * (s32)(FONT_ATLAS_DIMENSION / font->tile_height);

    font->atlas = allocate_string(FONT_ATLAS_DIMENSION * FONT_ATLAS_DIMENSION);

    init(&font->glyph_table, 1024);

    // TODO: proper loading of needed characters
    for (s32 i = 20; i < 128; i += 1) {
        font_add_glyph(font, i);
    }
    font_add_glyph(font, 0x5E, 0x1B);
}

void destroy(Font *font) {
    destroy_string(&font->atlas);
    destroy(&font->glyph_table);
}

GlyphInfo *get_glyph(Font *font, u32 cp) {
    GlyphInfo *info = find(&font->glyph_table, cp);
    if (!info) info = font_add_glyph(font, cp);
    return info;
}

r32 text_width(Font *font, String text) {
    r32 width = 0.0f;

    u8 *ptr = text.data;
    u8 *end = text.data + text.size;
    while (ptr != end) {
        u32 cp = *ptr;
        ptr += 1; // TODO: utf8
        GlyphInfo *info = get_glyph(font, cp);

        width += info->advance;
    }
    return width;
}

V2 font_text_metrics(Font *font, String text) {
    V2 result = {0.0f, font->line_advance};

    u8 *ptr = text.data;
    u8 *end = text.data + text.size;
    r32 width = 0.0f;
    while (ptr != end) {
        u32 cp = ptr[0];
        ptr += 1; // TODO: utf8

        if (cp == '\n') {
            result.y += font->line_advance;
            if (width > result.x) result.x = width;
            width = 0.0f;
            continue;
        }
        GlyphInfo *info = get_glyph(font, cp);

        width += info->advance;
    }
    if (width > result.x) result.x = width;
    return result;
}

