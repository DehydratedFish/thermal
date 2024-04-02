#include "thermal.h"
#include "platform.h"
#include "io.h"
#include "string2.h"
#include "memory.h"
#include "font.h"
#include "renderer.h"
#include "ui.h"

#include "ansi_escape_parser.h"
    


s32 const DefaultConsoleBufferSize = KILOBYTES(4);


V2i local_cursor_pos(ConsoleBuffer *buffer) {
    V2i result = {};
    if (buffer->lines.size > buffer->tile_count.y) {
        result.y = (buffer->scrollback_cursor.y + buffer->tile_count.y + buffer->scroll_offset) - buffer->lines.size;
    } else {
        result.y = buffer->scrollback_cursor.y;
    }

    result.x = buffer->scrollback_cursor.x; // TODO: - horizontal scroll_offset;

    return result;
}

INTERNAL Array<ConsoleTile> console_buffer_content(ConsoleBuffer *buffer) {
    u8 *end = (u8*)buffer->ring.memory + buffer->ring.end;
    u8 *ptr = end - buffer->ring.size;

    if (ptr < buffer->ring.memory) ptr += buffer->ring.alloc;

    Array<ConsoleTile> result = {};
    result.memory = (ConsoleTile*)(ptr);
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

INTERNAL u8 *end_ptr(ConsoleBuffer *buffer) {
    return (u8*)buffer->ring.memory + buffer->ring.end;
}

INTERNAL s32 offset_from_pointer(ConsoleBuffer *buffer, void *ptr) {
    u8 *p = (u8*)ptr;

    u8 *end = (u8*)buffer->ring.memory + buffer->ring.end;
    if (end < p) {
        end += buffer->ring.alloc;
    }

    assert(p <= end);
    return end - p;
}

void update_lines(ConsoleBuffer *buffer) {
    buffer->lines.size = 0;

    Array<ConsoleTile> content = console_buffer_content(buffer);

    LineInfo info = {content.memory, 0};

    LineInfo *current_line = append(buffer->lines, info);

    ConsoleTile *cursor_ptr = (ConsoleTile*)(end_ptr(buffer) - buffer->write_offset);
    for (s64 i = 0; i < content.size; i += 1) {
        if (content.memory + i == cursor_ptr) {
            buffer->scrollback_cursor.x = current_line->line.size;
            buffer->scrollback_cursor.y = buffer->lines.size - 1;
        }

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

    if (content.memory + content.size == cursor_ptr) {
        buffer->scrollback_cursor.x = current_line->line.size;
        buffer->scrollback_cursor.y = buffer->lines.size - 1;
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

INTERNAL Array<ConsoleTile> current_line(ConsoleBuffer *buffer) {
    return buffer->lines[buffer->scrollback_cursor.y].line;
}

INTERNAL void flush_conversion_buffer(ConsoleBuffer *buffer, s32 count) {
    u8 *ptr  = (u8*)buffer->conversion_buffer.memory;
    s64 size = count * sizeof(ConsoleTile);

    auto *write_func = platform_writable_range;

    if (buffer->write_offset) {
        s32 override_range = current_line(buffer).size - buffer->scrollback_cursor.x;
        if (override_range) {
            if (override_range > size) override_range = size;

            String range = platform_writable_range(&buffer->ring, size, buffer->write_offset);
            copy_memory(range.data, ptr, range.size);

            size -= range.size;
            ptr  += range.size;

            buffer->write_offset = offset_from_pointer(buffer, range.data + range.size);

            if (size == 0) return;
        }

        write_func = platform_writable_range_inserted;
    }

    String range = write_func(&buffer->ring, size, buffer->write_offset);

    while (size) {
        assert(size % sizeof(ConsoleTile) == 0);

        copy_memory(range.data, ptr, range.size);
        buffer->write_offset = offset_from_pointer(buffer, range.data + range.size);

        ptr  += range.size;
        size -= range.size;

        range = write_func(&buffer->ring, size, buffer->write_offset);
    }
}

u32 const DefaultTileFlags = 0;

INTERNAL void change_buffer_graphics(ConsoleBuffer *buffer, EscapeSequence seq) {
    for (s32 i = 0; i < seq.arg_count; i += 1) {
        switch (seq.args[i]) {
        case ESCAPE_CSI_RESET: {
           buffer->current_fg = buffer->fg_color;
           buffer->current_bg = buffer->bg_color;
           buffer->current_tile_flags = DefaultTileFlags;
        } break;

        case ESCAPE_CSI_SET_BOLD: buffer->current_tile_flags |= CONSOLE_TILE_FLAG_BOLD; break;

        case ESCAPE_CSI_FOREGROUND: {
            assert(i + 1 < seq.arg_count);

            if (seq.args[i + 1] == 5) {
                assert(i + 2 < seq.arg_count);
                assert(seq.args[i + 2] < 256);

                buffer->current_fg = ansi_8bit_color(seq.args[i + 2]);

                i += 1;
            } else if (seq.args[i + 1] == 2) {
                assert(i + 4 < seq.arg_count);

                s32 r = seq.args[i + 2];
                s32 g = seq.args[i + 3];
                s32 b = seq.args[i + 4];
                assert(r < 256);
                assert(g < 256);
                assert(b < 256);

                buffer->current_fg = PACK_RGB(r, g, b);

                i += 3;
            } else {
                LOG(LOG_ERROR, "Unsupported foreground color sequence.");
            }
        } break;

        case ESCAPE_CSI_BACKGROUND: {
        } break;

        case ESCAPE_CSI_FOREGROUND_BLACK:
        case ESCAPE_CSI_FOREGROUND_RED:
        case ESCAPE_CSI_FOREGROUND_GREEN:
        case ESCAPE_CSI_FOREGROUND_YELLOW:
        case ESCAPE_CSI_FOREGROUND_BLUE:
        case ESCAPE_CSI_FOREGROUND_MAGENTA:
        case ESCAPE_CSI_FOREGROUND_CYAN:
        case ESCAPE_CSI_FOREGROUND_WHITE:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_BLACK:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_RED:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_GREEN:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_YELLOW:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_BLUE:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_MAGENTA:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_CYAN:
        case ESCAPE_CSI_FOREGROUND_BRIGHT_WHITE:
            buffer->current_fg = ansi_4bit_color(seq.args[i]);
        break;

        case ESCAPE_CSI_BACKGROUND_BLACK:
        case ESCAPE_CSI_BACKGROUND_RED:
        case ESCAPE_CSI_BACKGROUND_GREEN:
        case ESCAPE_CSI_BACKGROUND_YELLOW:
        case ESCAPE_CSI_BACKGROUND_BLUE:
        case ESCAPE_CSI_BACKGROUND_MAGENTA:
        case ESCAPE_CSI_BACKGROUND_CYAN:
        case ESCAPE_CSI_BACKGROUND_WHITE:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_BLACK:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_RED:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_GREEN:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_YELLOW:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_BLUE:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_MAGENTA:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_CYAN:
        case ESCAPE_CSI_BACKGROUND_BRIGHT_WHITE:
            buffer->current_bg = ansi_4bit_color(seq.args[i]);
        break;
        }
    }
}

enum CursorMovement {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
};
INTERNAL void move_scrollback_cursor(ConsoleBuffer *buffer, CursorMovement direction, s32 amount) {
    V2i local_cursor = local_cursor_pos(buffer);

    if (direction == MOVE_UP) {
        if (local_cursor.y - amount < 0) amount = local_cursor.y;
        buffer->scrollback_cursor.y -= amount;
    } else if (direction == MOVE_DOWN) {
        if (local_cursor.y + amount > buffer->tile_count.y) amount = buffer->tile_count.y - local_cursor.y;
        buffer->scrollback_cursor.y += amount;
    } else if (direction == MOVE_LEFT) {
        if (local_cursor.x - amount < 0) amount = local_cursor.x;
        buffer->scrollback_cursor.x -= amount;
    } else if (direction == MOVE_RIGHT) {
        if (local_cursor.x + amount > buffer->tile_count.x) amount = buffer->tile_count.x - local_cursor.x;
        buffer->scrollback_cursor.x += amount;
    }

    if (buffer->line_wrap && buffer->scrollback_cursor.x > buffer->tile_count.x) {
        buffer->scrollback_cursor.x = buffer->tile_count.x;
    }

    LineInfo *line = &buffer->lines[buffer->scrollback_cursor.y];
    if (line->line.size < buffer->scrollback_cursor.x) {
        ConsoleTile space = {};
        space.cp = ' ';

        LineInfo *info = &buffer->lines[buffer->scrollback_cursor.y];
        s32 write_offset = offset_from_pointer(buffer, info->line.memory + info->line.size);

        // TODO: Only add spaces if there will actually be an insertion?
        s32 additional_tiles = (buffer->scrollback_cursor.x - line->line.size);
        String range = platform_writable_range_inserted(&buffer->ring, additional_tiles * sizeof(ConsoleTile), write_offset);

        ConsoleTile *mem = (ConsoleTile*)range.data;
        for (s32 i = 0; i < additional_tiles; i += 1) {
            mem[i] = space;
        }

        info->line.size += additional_tiles;
    }

    buffer->write_offset = offset_from_pointer(buffer, buffer->lines[buffer->scrollback_cursor.y].line.memory + buffer->scrollback_cursor.x);
}

INTERNAL void append(ConsoleBuffer *buffer, String str) {
    s32 conversion_count = 0;

    // TODO: SIMD scan over the buffer.

    while (str.size) {
        EscapeSequence seq = parse_escape_sequence(str);
        if (seq.valid) {
            flush_conversion_buffer(buffer, conversion_count);
            update_lines(buffer);

            str = shrink_front(str, seq.length_in_bytes);
            conversion_count = 0;

            if (seq.kind == 'm') {
                change_buffer_graphics(buffer, seq);
            } else if (seq.kind == 'K') {
                // TODO: erase to end of line, start of line and whole line
            } else if (seq.kind == 'A') {
                if (seq.args[0] == 0) seq.args[0] = 1;

                move_scrollback_cursor(buffer, MOVE_UP, seq.args[0]);
            } else if (seq.kind == 'B') {
                if (seq.args[0] == 0) seq.args[0] = 1;

                move_scrollback_cursor(buffer, MOVE_DOWN, seq.args[0]);
            } else if (seq.kind == 'C') {
                if (seq.args[0] == 0) seq.args[0] = 1;

                move_scrollback_cursor(buffer, MOVE_RIGHT, seq.args[0]);
            } else if (seq.kind == 'D') {
                if (seq.args[0] == 0) seq.args[0] = 1;

                move_scrollback_cursor(buffer, MOVE_LEFT, seq.args[0]);
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

INTERNAL void move_cursor_to_end(ConsoleBuffer *buffer) {
    buffer->scrollback_cursor.y = buffer->lines.size - 1;
    buffer->scrollback_cursor.x = buffer->lines[buffer->scrollback_cursor.y].line.size;

    buffer->write_offset = 0;
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
INTERNAL String ANSIColorTest =
"\x1b[38;2;255;0;255mThis is a test string that should look purple.\n"
"\x1b[0m\x1b[1mAnd this text is bold.\n"

"\x1b[0m"
"\x1b[30mBlack\n"
"\x1b[31mRed\n"
"\x1b[32mGreen\n"
"\x1b[33mYellow\n"
"\x1b[34mBlue\n"
"\x1b[35mMagenta\n"
"\x1b[36mCyan\n"
"\x1b[37mWhite\n"

"\x1b[90mBlack\n"
"\x1b[91mRed\n"
"\x1b[92mGreen\n"
"\x1b[93mYellow\n"
"\x1b[94mBlue\n"
"\x1b[95mMagenta\n"
"\x1b[96mCyan\n"
"\x1b[97mWhite\n"
;

INTERNAL String ANSICursorTest =
"sjfghsgskfjghl\x1b[3A\x1b[5CHallo\nT"
;


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
            buffer.write_offset = 0;

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
                } else if (command == "ansi_color") {
                    append(&buffer, ANSIColorTest);
                } else if (command == "ansi_cursor") {
                    append(&buffer, ANSICursorTest);
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

