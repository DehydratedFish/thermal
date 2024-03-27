#include "thermal.h"
#include "platform.h"
#include "io.h"
#include "string2.h"
#include "memory.h"
#include "font.h"
#include "renderer.h"
#include "ui.h"
    


INTERNAL s32 escape_number(String *str) {
    String num = {};
    num.data = str->data;

    for (s64 i = 0; i < str->size; i += 1) {
        if (str->data[i] >= '0' && str->data[i] <= '9') {
            num.size += 1;
        } else {
            break;
        }
    }

    *str = shrink_front(*str, num.size);
    s64 result = convert_string_to_s64(num.data, num.size);

    return result;
}

enum {
    ESC_SEQUENCE_NONE,

    ESC_SEQUENCE_GRAPHICS,

    ESC_SEQUENCE_ERASE,
};
struct EscapeSequence {
    u32 kind;
    s32 args[16];
    s32 arg_count;
    s32 length;
};

INTERNAL EscapeSequence parse_escape_sequence(String str) {
    EscapeSequence seq = {};

    if (str.size < 2) {
        return seq;
    }

    if (str.data[0] != 0x1B) return seq;
    if (str.data[1] != '[')  return seq;

    String start = str;

    str = shrink_front(str, 2);

    if (str.size && str[0] == 'K') {
        seq.kind   = ESC_SEQUENCE_ERASE;
        seq.length = 3;

        return seq;
    }

    seq.args[0] = escape_number(&str);
    seq.kind = ESC_SEQUENCE_GRAPHICS;

    s32 counter = 1;
    while (str.size && str.data[0] == ';') {
        str = shrink_front(str);

        s32 value = escape_number(&str);
        if (counter < 16) {
            seq.args[counter] = value;
        }

        counter += 1;
    }
    seq.arg_count = counter;

    if (str.size && str.data[0] != 'm') return {};

    seq.length = (str.data + 1) - start.data;

    return seq;
}

s32 const DefaultConsoleBufferSize = KILOBYTES(4);


INTERNAL Array<ConsoleTile> console_buffer_content(ConsoleBuffer *buffer) {
    u8 *ptr = (u8*)buffer->ring.memory + buffer->ring.alloc + buffer->ring.pos;

    Array<ConsoleTile> result = {};
    result.memory = (ConsoleTile*)(ptr - buffer->ring.size);
    result.size   = buffer->ring.size / sizeof(ConsoleTile);

    return result;
}

INTERNAL b32 eat_new_line(Array<ConsoleTile> content, s64 *index) {
    s64 i = *index;
    b32 result = false;

    if (content[i].cp == '\n') {
        i += 1;
        if (i < content.size && content[i].cp == '\r') i += 1;

        result = true;
    } else if (content[i].cp == '\r') {
        i += 1;
        if (i < content.size && content[i].cp == '\n') i += 1;

        result = true;
    }

    *index = i;

    return result;
}

void update_lines(ConsoleBuffer *buffer) {
    buffer->lines.size = 0;

    Array<ConsoleTile> content = console_buffer_content(buffer);
    if (content.size == 0) return;

    LineInfo info = {content.memory, 0};

    LineInfo *current_line = append(buffer->lines, info);

    for (s64 i = 0; i < content.size; i += 1) {
        if (eat_new_line(content, &i) ) {
            info.line.memory = content.memory + i;
            info.line.size = 0;
            current_line = append(buffer->lines, info);

            i -= 1;
        } else if (buffer->line_wrap && current_line->line.size == buffer->tile_count.x) {
            info.line.memory = content.memory + i;
            info.line.size = 1;
            current_line = append(buffer->lines, info);
        } else {
            current_line->line.size += 1;
        }
    }
}

INTERNAL Array<LineInfo> visible_lines(ConsoleBuffer *buffer) {
    s32 line_count = buffer->tile_count.y - 1;

    if (buffer->lines.size < line_count) {
        return buffer->lines;
    }

    s64 index = buffer->lines.size - (line_count + buffer->scroll_offset);
    Array<LineInfo> result = {};
    result.memory = buffer->lines.memory + index;
    result.size   = line_count;

    return result;
}

void update_display_buffer(ConsoleBuffer *buffer) {
    s32 line_count = buffer->tile_count.y;
    if (line_count < 1) return;

    if (buffer->tile_count.x * line_count != buffer->display_buffer.size) {
        prealloc(buffer->display_buffer, buffer->tile_count.x * line_count);
    }

    FOR (buffer->display_buffer, tile) {
        INIT_STRUCT(tile);
    }

    Array<LineInfo> lines = visible_lines(buffer);
    
    s32 line = 0;
    FOR (lines, info) {
        s64 size = info->line.size;
        if (size > buffer->tile_count.x) size = buffer->tile_count.x;

        copy_memory(&buffer->display_buffer[line * buffer->tile_count.x], info->line.memory, size * sizeof(ConsoleTile));
        line += 1;
    }
}


INTERNAL void flush_conversion_buffer(ConsoleBuffer *buffer, s32 count) {
    s64 size = count * sizeof(*buffer->conversion_buffer.memory);
    s32 written = platform_write(&buffer->ring, buffer->conversion_buffer.memory, size);
    assert(written == size);
}

u32 const DefaultTileFlags = 0;

INTERNAL void append(ConsoleBuffer *buffer, String str) {
    s32 conversion_count = 0;

    // TODO: SIMD scan over the buffer.

    while (str.size) {
        EscapeSequence seq = parse_escape_sequence(str);
        if (seq.kind != 0) {
            flush_conversion_buffer(buffer, conversion_count);

            str = shrink_front(str, seq.length);
            conversion_count = 0;

            if (seq.kind == ESC_SEQUENCE_GRAPHICS) {
                for (s32 i = 0; i < seq.arg_count; i += 1) {
                    if (seq.args[i] == 38 && seq.args[i + 1] == 2) {
                        buffer->current_fg = PACK_RGB(seq.args[i + 2], seq.args[i + 3], seq.args[i + 4]);
                        i += 4;
                    } else if (seq.args[i] == 48 && seq.args[i + 1] == 2) {
                        buffer->current_bg = PACK_RGB(seq.args[i + 2], seq.args[i + 3], seq.args[i + 4]);
                        i += 4;
                    } else if (seq.args[i] == 1) {
                        buffer->current_tile_flags |= CONSOLE_TILE_FLAG_BOLD;
                    } else if (seq.args[i] == 31) {
                        buffer->current_fg = PACK_RGB(255, 0, 0);
                    } else if (seq.args[i] == 0) {
                        buffer->current_fg = buffer->fg_color;
                        buffer->current_bg = buffer->bg_color;
                        buffer->current_tile_flags = DefaultTileFlags;
                    }
                }
            } else if (seq.kind == ESC_SEQUENCE_ERASE) {
                // TODO: erase to end of line, start of line and whole line
            }
        } else {
            UTF8CharResult c = utf8_peek(str);
            assert(c.status == 0);

            buffer->conversion_buffer[conversion_count].cp = c.cp;
            buffer->conversion_buffer[conversion_count].fg = buffer->current_fg;
            buffer->conversion_buffer[conversion_count].bg = buffer->current_bg;

            u32 style = CONSOLE_FONT_REGULAR;
            if ((buffer->current_tile_flags & CONSOLE_TILE_FLAGS_BOLD_ITALIC) == CONSOLE_TILE_FLAGS_BOLD_ITALIC) {
                style = CONSOLE_FONT_BOLD_ITALIC;
            } else if (buffer->current_tile_flags & CONSOLE_TILE_FLAG_BOLD) {
                style = CONSOLE_FONT_BOLD;
            } else if (buffer->current_tile_flags & CONSOLE_TILE_FLAG_ITALIC) {
                style = CONSOLE_FONT_ITALIC;
            }
            buffer->conversion_buffer[conversion_count].style = style;

            conversion_count += 1;

            if (conversion_count == buffer->conversion_buffer.size) {
                flush_conversion_buffer(buffer, conversion_count);
                conversion_count = 0;
            }

            str = shrink_front(str, c.length);
        }
    }
    flush_conversion_buffer(buffer, conversion_count);

    update_lines(buffer);
    update_display_buffer(buffer);
}

INTERNAL void generate_prompt(PromptBuffer *prompt, ApplicationState *state) {
    prompt->buffer_used = 0;

    for (UTF8Iterator it = make_utf8_it(prompt->format); it.valid; next(&it)) {
        if (prompt->buffer_used == PROMPT_BUFFER_SIZE) break;

        if (it.cp == '%') {
            next(&it);

            if (it.valid && it.cp == 'd') {
                prompt->kind = PROMPT_UPDATE_EACH_COMMAND;

                String dir = {state->current_dir.buffer.memory, state->current_dir.buffer.size};

                for (UTF8Iterator dir_it = make_utf8_it(dir); dir_it.valid; next(&dir_it)) {
                    if (prompt->buffer_used == PROMPT_BUFFER_SIZE) break;
                    
                    prompt->buffer[prompt->buffer_used] = dir_it.cp;
                    prompt->buffer_used += 1;
                }

                continue;
            }
        }

        prompt->buffer[prompt->buffer_used] = it.cp;
        prompt->buffer_used += 1;
    }
}

INTERNAL void backslash_to_slash(String str) {
    for (s64 i = 0; i < str.size; i += 1) {
        if (str[i] == '\\') str[i] = '/';
    }
}


s32 application_main(Array<String> args) {
    String starting_dir = platform_get_current_directory();

    ApplicationState state = {};
    change_path(&state.current_dir, starting_dir);
    state.data_dir = format("%S/data", platform_get_executable_path());
    state.running  = true;

    log_to_file(Console.out);

    platform_setup_window();
    ConsoleFont c_font = {};
    init(&c_font, 20.0f, t_format("%S/fonts", state.data_dir), "LiterationMono");



    Renderer renderer = {};
    init_renderer(&renderer);
    set_background(&renderer, {0.1f, 0.1f, 0.1f, 1.0f});

    M4 projection_2D;

    String liberation_mono = read_entire_file("data/fonts/LiterationMonoNerdFontMono-Regular.ttf");
    DEFER(destroy_string(&liberation_mono));
    Font font = {};
    init(&font, 20.0f, liberation_mono);

    GPUTexture font_texture = {};
    create_gpu_texture(&font_texture, {FONT_ATLAS_DIMENSION, FONT_ATLAS_DIMENSION}, 1, font.atlas, GPU_TEXTURE_FLAGS_NONE);

    UIState ui = {};
    init_ui(&ui, &font);
    ui.font_texture = &font_texture;

    ConsoleBuffer buffer = {};
    buffer.ring = platform_create_ring_buffer(DefaultConsoleBufferSize);
    assert(buffer.ring.alloc % sizeof(ConsoleTile) == 0);

    buffer.font = &c_font;

    buffer.fg_color  = PACK_RGB(210, 210, 210);
    buffer.bg_color  = 0;

    buffer.current_fg = buffer.fg_color;
    buffer.current_bg = buffer.bg_color;

    buffer.line_wrap = true;
    buffer.pipe_buffer = ALLOCATE_ARRAY(u8, MEGABYTES(16));

    buffer.conversion_buffer = ALLOCATE_ARRAY(ConsoleTile, 1024);
    assert((sizeof(*buffer.conversion_buffer.memory) * buffer.conversion_buffer.size) < buffer.ring.alloc);

    buffer.prompt.format = "%d > ";
    generate_prompt(&buffer.prompt, &state);


    while (state.running) {
        platform_update(&state);

        clear_background(&renderer);

        if (state.window_size_changed) {
            update_render_area(&renderer, state.window_size);

            projection_2D = ortho(state.window_size.width, state.window_size.height);
            update_2D_projection(&renderer, projection_2D);
        }

        begin_frame(&ui, state.window_size, &state.user_input);

        b32 command_run = console_buffer_view(&ui, &buffer, &buffer);
        if (command_run) {
            buffer.current_fg = buffer.fg_color;
            buffer.current_bg = buffer.bg_color;
            buffer.current_tile_flags = 0;

            String32 utf32_command = {buffer.command.memory, buffer.command.size};

            if (utf32_command.size) {
                buffer.scroll_offset = 0;

                // TODO: This fails if the command gets to long.
                //       Either limit the command length or use a DArray<u32> that will keep the memory
                //       allocated across command runs.
                String command = trim(to_utf8(temporary_allocator(), utf32_command));

                String32 utf32_prompt = {buffer.prompt.buffer, buffer.prompt.buffer_used};

                if (buffer.lines.size && buffer.lines[buffer.lines.size - 1].line.size != 0) {
                    append(&buffer, "\n");
                }

                String prompt = to_utf8(temporary_allocator(), utf32_prompt);
                append(&buffer, prompt);
                append(&buffer, command);
                append(&buffer, "\n");

                buffer.command.size = 0;

                if (command == "exit") {
                    state.running = false;
                } else if (command == "ls") {
                    auto listing = platform_directory_listing(".");

                    FOR (listing, dir) {
                        append(&buffer, *dir);
                        append(&buffer, "\n");
                    }
                } else if (starts_with(command, "cd ")) {
                    command = trim(shrink_front(command, 3));

                    if (!platform_change_directory(command)) {
                        append(&buffer, "Could not change directory.\n");
                    } else {
                        change_path(&state.current_dir, command);
                    }
                } else if (command == "ansi_test") {
                    append(&buffer, "\x1b[38;2;255;0;255mThis is a test string that should look purple. \x1b[0m\x1b[1mAnd this text is bold.\n");
                } else {
                    buffer.pec = platform_execute(command);

                    if (!buffer.pec.started_successfully) {
                        String message = "Error running command.\n";
                        append(&buffer, message);
                    }
                }
            }
        }

        end_frame(&ui);

        if (buffer.prompt.kind == PROMPT_UPDATE_EACH_COMMAND && command_run) {
            generate_prompt(&buffer.prompt, &state);
        }

        if (buffer.pec.started_successfully) {
            u32 pending = platform_input_available(&buffer.pec);
            while (pending) {
                s32 read = platform_read(&buffer.pec, buffer.pipe_buffer.memory, buffer.pipe_buffer.size);
                append(&buffer, {buffer.pipe_buffer.memory, read});

                pending = platform_input_available(&buffer.pec);
            }
        }

        if (c_font.is_dirty) {
            update_gpu_texture(&font_texture, c_font.atlas);

            c_font.is_dirty = false;
        }

        draw_ui(&renderer, &ui);
        platform_window_swap_buffers();

        reset_temporary_storage();
    }

    destroy_renderer(&renderer);

    return 0;
}

