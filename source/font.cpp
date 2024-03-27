#include "font.h"

#include "string2.h"
#include "io.h"

#include "platform.h"


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
    font->tile_width  = ceil(advance_width * font->scale);
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


INTERNAL ConsoleGlyphInfo *font_add_glyph(ConsoleFont *font, u32 kind, u32 cp, u32 mapping = 0);

ConsoleGlyphInfo *get_glyph(ConsoleFont *font, u32 kind, u32 cp) {
    ConsoleGlyphInfo *info = find(&font->glyph_table, cp);
    if (!info || !info->loaded[kind]) info = font_add_glyph(font, kind, cp);

    return info;
}

INTERNAL ConsoleGlyphInfo *font_add_glyph(ConsoleFont *font, u32 kind, u32 cp, u32 mapping) {
    u32 glyph_count = font->glyph_table.buckets;
    if (glyph_count == font->max_glyphs) {
        // TODO: path for when the cache is full
    } else {
        STBFont *stb = &font->font_data[kind];

        int glyph = stbtt_FindGlyphIndex(&stb->info, cp);
        if (glyph == 0) return get_glyph(font, kind, '?'); // TODO: proper replacement character

        int x0, x1, y0, y1;
        stbtt_GetGlyphBitmapBox(&stb->info, glyph, stb->scale, stb->scale, &x0, &y0, &x1, &y1);

        s32 height = (y1 - y0);
        s32 width  = (x1 - x0);

        int unscaled_advance_width, unscaled_lsb;
        stbtt_GetGlyphHMetrics(&stb->info, glyph, &unscaled_advance_width, &unscaled_lsb);

        r32 lsb = unscaled_lsb * stb->scale;
        r32 y_adjust = stb->ascent + y0;

        s32 x = font->next_free_glyph.x + (s32)lsb;
        s32 y = font->next_free_glyph.y + (s32)y_adjust;
        s32 pos = (y * FONT_ATLAS_DIMENSION) + x;
        stbtt_MakeGlyphBitmap(&stb->info, &font->atlas[pos], width, height, FONT_ATLAS_DIMENSION, stb->scale, stb->scale, glyph);

        V2i offset_in_atlas = font->next_free_glyph;

        font->next_free_glyph.x += font->glyph_width;
        if (font->next_free_glyph.x > FONT_ATLAS_DIMENSION) {
            font->next_free_glyph.x  = 0;
            font->next_free_glyph.y += font->glyph_height;
        }

        font->is_dirty = true;

        if (mapping != 0) cp = mapping;

        ConsoleGlyphInfo *result = find(&font->glyph_table, cp);
        if (result) {
            result->offset_in_atlas[kind] = offset_in_atlas;
            result->loaded[kind] = true;
        } else {
            ConsoleGlyphInfo info = {};
            info.offset_in_atlas[kind] = offset_in_atlas;
            info.loaded[kind] = true;

            result = insert(&font->glyph_table, cp, info);
        }

        return result;
    }
    return 0;
}

void init(ConsoleFont *font, r32 font_height, String font_dir, String font_family, Allocator alloc) {
    font->allocator = alloc;
    Array<String> fonts = platform_directory_listing(font_dir);

    // TODO: This is not really correct. I think the name of the type face is stored inside the font and
    //       not guessed from the file name. But dealing with getting the file names of registered fonts
    //       on windows is so stressful I don't want to deal with it right now.
    DArray<String> family = {};
    FOR (fonts, file) {
        if (starts_with(*file, font_family)) {
            append(family, *file);
        }
    }

    FOR (family, file) {
        stbtt_fontinfo stb = {};

        u32 status;
        String ttf = read_entire_file(t_format("%S/%S", font_dir, *file), &status, font->allocator);
        if (status != READ_ENTIRE_FILE_OK) {
            if (status == READ_ENTIRE_FILE_NOT_FOUND) {
                LOG(LOG_ERROR, "Could not find font %S\n", *file);
            } else if (status == READ_ENTIRE_FILE_READ_ERROR) {
                LOG(LOG_ERROR, "Could not read font %S\n", *file);
            }

            continue;
        }

        stbtt_InitFont(&stb, ttf.data, stbtt_GetFontOffsetForIndex(ttf.data, 0));
        r32 scale = stbtt_ScaleForPixelHeight(&stb, font_height);

        u16 const bold   = 0x01;
        u16 const italic = 0x02;
        u16 const bold_italic = bold | italic;
        u16 mac_style = *(u16*)(ttf.data + stb.head + 45);

        u32 kind = CONSOLE_FONT_REGULAR;
        if ((mac_style & bold_italic) == bold_italic) {
            kind = CONSOLE_FONT_BOLD_ITALIC;
        } else if (mac_style & bold) {
            kind = CONSOLE_FONT_BOLD;
        } else if (mac_style & italic) {
            kind = CONSOLE_FONT_ITALIC;
        }

        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&stb, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        r32 ascent  = unscaled_ascent * scale;
        r32 descent = unscaled_descent * scale;

        int unscaled_advance_width, unscaled_lsb;
        stbtt_GetCodepointHMetrics(&stb, ' ', &unscaled_advance_width, &unscaled_lsb);

        s32 width  = (s32)ceil(unscaled_advance_width * scale);
        s32 height = (s32)ceil(ascent - descent);

        if (font->glyph_width  < width)  font->glyph_width  = width;
        if (font->glyph_height < height) font->glyph_height = height;

        STBFont data = {};
        data.data    = ttf;
        data.info    = stb;
        data.scale   = scale;
        data.ascent  = ascent;
        data.descent = descent;

        font->font_data[kind] = data;
    }

    // TODO: Check if all or at least the regular font are loaded.

    init(&font->glyph_table, 1024);
    font->atlas = allocate_string(FONT_ATLAS_DIMENSION * FONT_ATLAS_DIMENSION);

    font->glyphs_per_line = (s32)(FONT_ATLAS_DIMENSION / font->glyph_width);
    font->max_glyphs      = font->glyphs_per_line * (s32)(FONT_ATLAS_DIMENSION / font->glyph_height);

    // TODO: proper loading of needed characters
    for (s32 i = 20; i < 128; i += 1) {
        font_add_glyph(font, CONSOLE_FONT_REGULAR, i);
    }
    font_add_glyph(font, CONSOLE_FONT_REGULAR, 0x5E, 0x1B);
}

