#include "ui.h"

#include "memory.h"
#include "font.h"
#include "string2.h"
#include "utf.h"

#include "opengl.h"


void update_ui_state(UIState *ui, V2 window_size, UserInput *input) {
    ui->input = *input;
    ui->draw_region = window_size;
    ui->default_window.dim = window_size;
}

INTERNAL void add_vertex(UIState *ui, UIVertex vertex) {
    UIWindow *window = ui->current_window;
    assert(window->tasks.size != 0);

    append(ui->vertices, vertex);
    last(window->tasks)->count += 1;
}

INTERNAL void add_text_vertex(UIState *ui, UITextVertex vertex) {
    UIWindow *window = ui->current_window;
    assert(window->tasks.size != 0);

    append(ui->text_vertices, vertex);
    last(window->tasks)->count += 1;
}

INTERNAL b32 mouse_released(UIState *ui) {
    return ui->input.last_mouse.lmb && !ui->input.mouse.lmb;
}

INTERNAL void new_task(UIState *ui, UITaskKind kind, GPUTexture *texture) {
    UIWindow *window = ui->current_window;

    UITask *last_task = last(window->tasks);
    if (last_task && last_task->texture == texture && last_task->kind == kind) return;

    UITask task  = {};
    task.kind    = kind;
    task.texture = texture;
    task.offset  = ui->vertices.size;

    append(window->tasks, task);
}

struct WidgetMouseState {
    b32 hovered;
    b32 last_clicked;

    b32 pressed;
    b32 held;
    b32 released;
};

INTERNAL WidgetMouseState register_hittest(UIState *ui, void *id, UIRect region) {
    UIHittest hit = {};
    hit.id = id;
    hit.region = region;

    append(ui->current_window->hittests, hit);

    WidgetMouseState state = {};
    if (ui->hover == id) {
        state.hovered = true;

        if (!ui->input.last_mouse.lmb && ui->input.mouse.lmb) {
            state.pressed = true;
            ui->last_clicked = id;
        } else if (ui->input.last_mouse.lmb && ui->input.mouse.lmb) {
            state.held = true;
        } else if (ui->input.last_mouse.lmb && !ui->input.mouse.lmb) {
            state.released = true;
        }
    }

    if (ui->last_clicked == id) {
        state.last_clicked = true;
    }

    return state;
}

INTERNAL b32 cursor_in_rect(V2 p, UIRect r) {
    r32 x1 = r.x + r.w;
    r32 y1 = r.y + r.h;

    return p.x >= r.x && p.y >= r.y && p.x < x1 && p.y < y1;
}

INTERNAL void *find_hovered_id(UIState *ui) {
    FOR (ui->windows, window) {
        for (s64 i = window->hittests.size; i > 0; i -= 1) {
            UIHittest hit = window->hittests[i - 1];

            if (cursor_in_rect(ui->input.mouse.cursor, hit.region)) return hit.id;
        }
    }

    for (s64 i = ui->default_window.hittests.size; i > 0; i -= 1) {
        UIHittest hit = ui->default_window.hittests[i - 1];

        if (cursor_in_rect(ui->input.mouse.cursor, hit.region))
            return hit.id;
    }

    return 0;
}

INTERNAL UIPersistentData *get_widget_data(UIState* ui, void *id) {
    FOR (ui->widget_data, data) {
        if (data->id == id) return data;
    }

    UIPersistentData data = {};
    data.id = id;

    return append(ui->widget_data, data);
}

INTERNAL void prepare_for_new_frame(UIWindow *window) {
    window->hittests.size = 0;
    window->tasks.size = 0;

    window->next_widget_pos.x = window->ui->margin;
    window->next_widget_pos.y = window->ui->margin;
}

INTERNAL UIRect expand_rect(UIRect rect, r32 amount) {
    rect.x -= amount;
    rect.y -= amount;
    rect.w += amount;
    rect.h += amount;

    return rect;
}

INTERNAL UIRect next_widget_region(UIWindow *window, V2 default_size) {
    UIRect result = {};
    result.x = window->next_widget_pos.x;
    result.y = window->next_widget_pos.y;

    r32 margin = window->ui->margin;

    if (window->layout.kind == UI_LAYOUT_NONE) {
        result.w = default_size.width;
        result.h = default_size.height;

        window->next_widget_pos.x  = margin;
        window->next_widget_pos.y += default_size.y + margin;
    } else if (window->layout.kind == UI_LAYOUT_ROW) {
        result.h = window->layout.dim.height;

        if (window->layout.dim.width == 0.0f) {
            result.w = default_size.width;
        } else {
            result.w = window->layout.dim.width;
        }

        window->next_widget_pos.x += result.w + margin;

        window->layout.column += 1;
        if (window->layout.column == window->layout.columns) {
            window->next_widget_pos.x  = window->ui->margin;
            window->next_widget_pos.y += window->layout.dim.height + window->ui->margin;
            INIT_STRUCT(&window->layout);
        }
    } else if (window->layout.kind == UI_LAYOUT_WIDGET) {
        result.w = window->layout.dim.width;
        result.h = window->layout.dim.height;

        INIT_STRUCT(&window->layout);
    }

    return result;
}


void init_ui(UIState *ui, Font *font) {
    ui->default_window.ui = ui;
    ui->current_window    = &ui->default_window;

    Array<VertexBinding> bindings = allocate_array<VertexBinding>(4, temporary_allocator());
    bindings[0] = {VERTEX_COMPONENT_V3, ShaderPositionLocation, STRUCT_OFFSET(UITextVertex, pos)};
    bindings[1] = {VERTEX_COMPONENT_V2, ShaderUVLocation      , STRUCT_OFFSET(UITextVertex, uv)};
    bindings[2] = {VERTEX_COMPONENT_PACKED_COLOR, ShaderForegroundColorLocation, STRUCT_OFFSET(UITextVertex, fg)};
    bindings[3] = {VERTEX_COMPONENT_PACKED_COLOR, ShaderBackgroundColorLocation, STRUCT_OFFSET(UITextVertex, bg)};

    create_gpu_buffer(&ui->text_vertex_buffer, sizeof(UITextVertex), 0, 0, bindings);

    create_gpu_shader(&ui->text_shader, "text.glsl");
}


void destroy_ui(UIState *ui) {
    destroy(ui->default_window.tasks);
    destroy(ui->default_window.hittests);

    FOR (ui->windows, window) {
        destroy(window->tasks);
        destroy(window->hittests);
    }

    destroy(ui->windows);
    destroy(ui->vertices);
    destroy(ui->widget_data);
}

void begin_frame(UIState *ui, V2 window_size, UserInput *input) {
    ui->hover = find_hovered_id(ui);
    ui->draw_region = window_size;
    ui->default_window.dim = window_size;
    ui->input = *input;

    prepare_for_new_frame(&ui->default_window);

    ui->vertices.size = 0;
    ui->text_vertices.size = 0;
}

void end_frame(UIState *ui) {
    if (mouse_released(ui)) ui->last_clicked = 0;
}

void row_layout(UIState *ui, r32 height, s32 columns, UILayoutRowKind kind) {
    UIWindow *window = ui->current_window;

    UILayout layout = {};

    layout.kind    = UI_LAYOUT_ROW;
    layout.columns = columns;
    layout.dim.height = height;

    if (kind == UI_LAYOUT_EVEN) {
        layout.total_width = window->dim.width - ui->margin - ui->margin;

        layout.dim.width = (window->dim.width - (ui->margin * (columns + 1))) / columns;
    } else if (kind == UI_LAYOUT_EXACT) {
        layout.dim.width = 0.0f;
    }

    window->layout = layout;
}

void column_width(UIState *ui, r32 width) {
    UILayout *layout = &ui->current_window->layout;
    if (layout->kind == UI_LAYOUT_NONE) return;

    if (width == 0.0f) {
        layout->dim.width = layout->total_width - layout->used_width;
    } else {
        layout->dim.width = width;
    }
}

/*
void draw_text(UIState *ui, UIRect region, Color color, String text, u32 align) {
    new_task(ui, UI_TASK_REGULAR, ui->font_texture);

    r32 x, baseline;

    V2 text_dims = font_text_metrics(ui->font, text);

    if (align == UI_TEXT_TOP_LEFT) {
        x        = region.x;
        baseline = region.y + ui->font->ascent;
    } else if (align == UI_TEXT_CENTER_LEFT) {
        x        = region.x;
        baseline = (region.y + (region.h / 2.0f)) - (text_dims.height / 2.0f) + ui->font->ascent;
    } else if (align == UI_TEXT_CENTER_CENTER) {
        x        = (region.x + (region.w / 2.0f)) - (text_dims.width / 2.0f);
        baseline = (region.y + (region.h / 2.0f)) - (text_dims.height / 2.0f) + ui->font->ascent;
    }

    for (UTF8Iterator it = make_utf8_it(text); it.valid; next(&it)) {
        GlyphInfo *glyph = get_glyph(ui->font, it.cp);

        r32 x0 = floor(glyph->x0 + x);
        r32 y0 = floor(glyph->y0 + baseline);
        r32 x1 = floor(glyph->x1 + x);
        r32 y1 = floor(glyph->y1 + baseline);

        add_vertex(ui, {{x0, y0}, {glyph->u0, glyph->v0}, color});
        add_vertex(ui, {{x1, y0}, {glyph->u1, glyph->v0}, color});
        add_vertex(ui, {{x0, y1}, {glyph->u0, glyph->v1}, color});

        add_vertex(ui, {{x0, y1}, {glyph->u0, glyph->v1}, color});
        add_vertex(ui, {{x1, y0}, {glyph->u1, glyph->v0}, color});
        add_vertex(ui, {{x1, y1}, {glyph->u1, glyph->v1}, color});

        x += glyph->advance;
    }
}

INTERNAL void draw_rect(UIState *ui, UIRect rect, Color color) {
    new_task(ui, UI_TASK_REGULAR, 0);

    add_vertex(ui, {{rect.x, rect.y}, {}, color});
    add_vertex(ui, {{rect.x + rect.w, rect.y}, {}, color});
    add_vertex(ui, {{rect.x, rect.y + rect.h}, {}, color});

    add_vertex(ui, {{rect.x, rect.y + rect.h}, {}, color});
    add_vertex(ui, {{rect.x + rect.w, rect.y}, {}, color});
    add_vertex(ui, {{rect.x + rect.w, rect.y + rect.h}, {}, color});
}

void text_label(UIState *ui, String text) {
    V2 const text_dims = font_text_metrics(ui->font, text);
    
    UIRect rect = next_widget_region(ui->current_window, text_dims);

    draw_text(ui, rect, TextForeground, text, UI_TEXT_CENTER_LEFT);
}

b32 button(UIState *ui, void *id, String text, bool active) {
    r32 const text_margin = 10.0f;

    // TODO: text metrics get calculated twice (2nd time in the call to draw_text)
    V2 default_size = font_text_metrics(ui->font, text);
    default_size.width  += text_margin;
    default_size.height += text_margin;

    b32 clicked = false;

    UIRect rect = next_widget_region(ui->current_window, default_size);

    if (!active) {
        draw_rect(ui, rect, ButtonInactiveBackground);
        draw_text(ui, rect, TextForeground, text, UI_TEXT_CENTER_CENTER);

        return false;
    }

    WidgetMouseState state = register_hittest(ui, id, rect);

    Color color = ButtonBackground;
    if (state.hovered) {
        color = ButtonHovered;

        if (state.released) {
            clicked = true;
        }
    }

    if (state.last_clicked) {
        color = ButtonClicked;
    }

    draw_rect(ui, rect, color);
    draw_text(ui, rect, TextForeground, text, UI_TEXT_CENTER_CENTER);

    return clicked;
}

b32 button(UIState *ui, String text, bool active) {
    return button(ui, text.data, text, active);
}

b32 expandable_window_seperator(UIState *ui, void *id, String label) {
    UIWindow *window = ui->current_window;
    UIPersistentData *widget_data = get_widget_data(ui, id);
    
    window->next_widget_pos.x  = 0.0f;

    INIT_STRUCT(&window->layout);
    r32 width  = ui->current_window->dim.width;
    r32 height = 30.0f;

    UIRect rect = {};
    rect.x = window->next_widget_pos.x;
    rect.y = window->next_widget_pos.y;
    rect.w = width;
    rect.h = height;

    WidgetMouseState state = register_hittest(ui, id, rect);
    if (state.hovered && state.released) widget_data->active = !widget_data->active;

    draw_rect(ui, rect, ButtonBackground);
    window->next_widget_pos.x = ui->margin;

    row_layout(ui, height, 2, UI_LAYOUT_EXACT);

    if (widget_data->active) {
        text_label(ui, "-");
    } else {
        text_label(ui, "+");
    }

    text_label(ui, label);

    window->next_widget_pos.x = ui->margin;

    return widget_data->active;
}
*/



/*
INTERNAL void draw_character(UIState *ui, V2 offset, V2i tile, u32 cp, u32 fg, u32 bg) {
    GlyphInfo *glyph = get_glyph(ui->font, cp);

    r32 x0 = offset.x + tile.x * ui->font->tile_width;
    r32 y0 = offset.y + tile.y * ui->font->tile_height;
    r32 x1 = x0 + ui->font->tile_width;
    r32 y1 = y0 + ui->font->tile_height;

    add_text_vertex(ui, {{x0, y0}, {glyph->u0, glyph->v0}, fg, bg});
    add_text_vertex(ui, {{x1, y0}, {glyph->u1, glyph->v0}, fg, bg});
    add_text_vertex(ui, {{x0, y1}, {glyph->u0, glyph->v1}, fg, bg});

    add_text_vertex(ui, {{x0, y1}, {glyph->u0, glyph->v1}, fg, bg});
    add_text_vertex(ui, {{x1, y0}, {glyph->u1, glyph->v0}, fg, bg});
    add_text_vertex(ui, {{x1, y1}, {glyph->u1, glyph->v1}, fg, bg});
}
*/

INTERNAL void draw_character(UIState *ui, ConsoleFont *font, V2 offset, V2i tile, u32 cp, u32 kind, u32 fg, u32 bg) {
    ConsoleGlyphInfo *glyph = get_glyph(font, kind, cp);

    r32 x0 = offset.x + tile.x * font->glyph_width;
    r32 y0 = offset.y + tile.y * font->glyph_height;
    r32 x1 = x0 + font->glyph_width;
    r32 y1 = y0 + font->glyph_height;

    r32 u0 = glyph->offset_in_atlas[kind].x * FONT_ATLAS_RATIO;
    r32 v0 = glyph->offset_in_atlas[kind].y * FONT_ATLAS_RATIO;
    r32 u1 = (glyph->offset_in_atlas[kind].x + font->glyph_width)  * FONT_ATLAS_RATIO;
    r32 v1 = (glyph->offset_in_atlas[kind].y + font->glyph_height) * FONT_ATLAS_RATIO;

    add_text_vertex(ui, {{x0, y0}, {u0, v0}, fg, bg});
    add_text_vertex(ui, {{x1, y0}, {u1, v0}, fg, bg});
    add_text_vertex(ui, {{x0, y1}, {u0, v1}, fg, bg});

    add_text_vertex(ui, {{x0, y1}, {u0, v1}, fg, bg});
    add_text_vertex(ui, {{x1, y0}, {u1, v0}, fg, bg});
    add_text_vertex(ui, {{x1, y1}, {u1, v1}, fg, bg});
}

INTERNAL void append_code_point(DArray<u8> *cmd, u32 cp) {
    UTF8CharResult result = to_utf8(cp);
    assert(result.status == 0);

    append(*cmd, result.byte, result.length);
}

INTERNAL u32 remove_last_code_point(DArray<u8> *cmd) {
    s32 length = 0;

    while (cmd->size) {
        if ((cmd->memory[cmd->size - 1] & 0xC0) == 0x80) {
            cmd->size -= 1;
            length += 1;
        } else {
            cmd->size -= 1;
            length += 1;
            break;
        }
    }

    if (length) {
        UTF8CharResult last_cp = utf8_peek({cmd->memory, length});
        assert(last_cp.status == GET_OK);

        return last_cp.cp;
    }

    return 0;
}

INTERNAL u32 invert_rgb(u32 color) {
    return (0x00FFFFFF - (color | 0xFF000000)) | (color & 0xFF000000);
}

b32 console_buffer_view(UIState *ui, void *id, ConsoleBuffer *buffer) {
    if (ui->focus == 0) {
        ui->focus = id;
    }

    UIRect region = next_widget_region(ui->current_window, ui->draw_region);
    V2i tile_count = {(s32)(region.w / buffer->font->glyph_width), (s32)(region.h / buffer->font->glyph_height)};
    if (tile_count.x != buffer->tile_count.x || tile_count.y != buffer->tile_count.y) {
        buffer->tile_count = tile_count;

        update_lines(buffer);
        update_display_buffer(buffer);
    }

    if (tile_count.y < 2) {
        // TODO: skip rendering and just process input if the window is too small.
        return false;
    }

    new_task(ui, UI_TASK_TEXT, ui->font_texture);

    V2 padding = {region.w - (buffer->font->glyph_width * tile_count.x), region.h - (buffer->font->glyph_height * tile_count.y)};

    V2 offset = {
        floorf(region.x + (padding.x * 0.5f)),
        floorf(region.y + (padding.y * 0.5f))
    };

    u32 fg = buffer->fg_color;
    u32 bg = 0;

    s32 pos = buffer->ring.pos;
    if (pos < buffer->display_start) pos += buffer->ring.alloc;

    String content = {
        (u8*)buffer->ring.memory + buffer->display_start,
        pos - buffer->display_start
    };

    tile_count.y -= 1;

    V2i current_tile = {};
    FOR (buffer->display_buffer, tile) {
        if (tile->cp != 0) {
            draw_character(ui, buffer->font, offset, current_tile, tile->cp, tile->style, tile->fg, tile->bg);
        }

        current_tile.x += 1;
        if (current_tile.x == buffer->tile_count.x) {
            current_tile.y += 1;
            current_tile.x  = 0;
        }
    }

    current_tile.y = tile_count.y;
    current_tile.x = 0;

    b32 fire_command = false;

    UserInput *input = &ui->input;
    for (s32 i = 0; i < input->key_buffer_used; i += 1) {
        KeyPress *key = &input->key_buffer[i];

        if (key->key == KEY_BACKSPACE) {
            if (input->ctrl_held) {
                s64 previous_pos = buffer->cursor_pos;

                while (buffer->cursor_pos > 0) {
                    buffer->cursor_pos -= 1;
                    if (buffer->command[buffer->cursor_pos] != ' ') break;
                }

                while (buffer->cursor_pos > 0) {
                    buffer->cursor_pos -= 1;
                    if (buffer->command[buffer->cursor_pos] == ' ') break;
                }

                s64 moved = previous_pos - buffer->cursor_pos;
                stable_remove(buffer->command, buffer->cursor_pos, moved);
            } else {
                if (buffer->cursor_pos > 0) {
                    buffer->cursor_pos -= 1;
                    stable_remove(buffer->command, buffer->cursor_pos);
                }
            }
        } else if (key->key == KEY_DELETE) {
            if (input->ctrl_held) {
                s64 pos = buffer->cursor_pos;

                while (pos < buffer->command.size) {
                    if (buffer->command[pos] != ' ') break;
                    pos += 1;
                }

                while (pos < buffer->command.size) {
                    if (buffer->command[pos] == ' ') {
                        pos += 1;

                        break;
                    }
                    pos += 1;
                }

                s64 moved = pos - buffer->cursor_pos;
                stable_remove(buffer->command, buffer->cursor_pos, moved);
            } else {
                if (buffer->cursor_pos < buffer->command.size) {
                    stable_remove(buffer->command, buffer->cursor_pos);
                }
            }
        } else if (key->key == KEY_RETURN) {
            buffer->cursor_pos = 0;
            fire_command = true;
        } else if (key->key == KEY_LEFT_ARROW) {
            if (input->ctrl_held) {
                while (buffer->cursor_pos > 0) {
                    buffer->cursor_pos -= 1;
                    if (buffer->command[buffer->cursor_pos] != ' ') break;
                }

                while (buffer->cursor_pos > 0) {
                    if (buffer->command[buffer->cursor_pos - 1] == ' ') break;
                    buffer->cursor_pos -= 1;
                }
            } else {
                if (buffer->cursor_pos > 0) buffer->cursor_pos -= 1;
            }
        } else if (key->key == KEY_RIGHT_ARROW) {
            if (input->ctrl_held) {
                while (buffer->cursor_pos < buffer->command.size) {
                    buffer->cursor_pos += 1;
                    if (buffer->command[buffer->cursor_pos] != ' ') break;
                }

                while (buffer->cursor_pos < buffer->command.size) {
                    if (buffer->command[buffer->cursor_pos] == ' ') break;
                    buffer->cursor_pos += 1;
                }
            } else {
                if (buffer->cursor_pos < buffer->command.size) buffer->cursor_pos += 1;
            }
        } else if (key->key == KEY_HOME) {
            buffer->cursor_pos = 0;
        } else if (key->key == KEY_END) {
            buffer->cursor_pos = buffer->command.size;
        } else {
            insert(buffer->command, buffer->cursor_pos, key->code_point);
            buffer->cursor_pos += 1;
        }
    }

    if (input->mouse.scroll) {
        u32 const lines_to_scroll = 3;

        buffer->scroll_offset += input->mouse.scroll * lines_to_scroll;
        if (buffer->scroll_offset + buffer->tile_count.y > buffer->lines.size) {
            buffer->scroll_offset = buffer->lines.size - buffer->tile_count.y;
        }

        if (buffer->scroll_offset < 0) buffer->scroll_offset = 0;

        update_display_buffer(buffer);
    }

    for (s64 i = 0; i < buffer->prompt.buffer_used; i += 1) {
        draw_character(ui, buffer->font, offset, current_tile, buffer->prompt.buffer[i], CONSOLE_FONT_REGULAR, fg, bg);
        current_tile.x += 1;
    }

    s64 index = 0;
    // TODO: Skip beginning if too long or add command lines at the bottom
    for (;index < buffer->command.size; index += 1) {
        if (index == buffer->cursor_pos) {
            draw_character(ui, buffer->font, offset, current_tile, buffer->command[index], CONSOLE_FONT_REGULAR, invert_rgb(fg), invert_rgb(bg));
        } else {
            draw_character(ui, buffer->font, offset, current_tile, buffer->command[index], CONSOLE_FONT_REGULAR, fg, bg);
        }

        current_tile.x += 1;
    }
    if (index == buffer->cursor_pos) {
        draw_character(ui, buffer->font, offset, current_tile, ' ', CONSOLE_FONT_REGULAR, invert_rgb(fg), invert_rgb(bg));
    }

    return fire_command;
}

INTERNAL void draw_window(Renderer *renderer, UIWindow *window) {
    FOR (window->tasks, task) {
        if (task->texture) {
            glBindTexture(GL_TEXTURE_2D, task->texture->id);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDrawArrays(GL_TRIANGLES, task->offset, task->count);
    }
}

// TODO: This is very hand crafty to make it work as I don't want to iterate
//       over the task array twice and converting UITasks to RenderTasks.
//       But using RenderTasks inside the UI directly would make it a hastle
//       to check if two tasks can be combined.
//       Can this be made a bit better?
void draw_ui(Renderer *renderer, UIState *ui) {
    glUseProgram(ui->text_shader.id);
    update_gpu_buffer(&ui->text_vertex_buffer, ui->text_vertices.memory, ui->text_vertices.size);

    glBindVertexArray(ui->text_vertex_buffer.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ui->text_vertex_buffer.vbo);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    draw_window(renderer, &ui->default_window);
    FOR (ui->windows, window) {
        draw_window(renderer, window);
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

