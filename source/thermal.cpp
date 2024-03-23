#include "thermal.h"
#include "platform.h"
#include "io.h"
#include "string2.h"
#include "memory.h"
#include "font.h"
#include "renderer.h"
#include "ui.h"
    


s32 const DefaultConsoleBufferSize = KILOBYTES(4);


INTERNAL String console_buffer_content(ConsoleBuffer *buffer) {
    u8 *ptr = (u8*)buffer->ring.memory + buffer->ring.alloc + buffer->ring.pos;

    String result = {};
    result.data = ptr - buffer->ring.size;
    result.size = buffer->ring.size;

    return result;
}

INTERNAL b32 eat_new_line(UTF8Iterator *it) {
    if (it->cp == '\n') {
        next(it);
        if (it->cp == '\r') next(it);

        return true;
    } else if (it->cp == '\r') {
        next(it);
        if (it->cp == '\n') next(it);

        return true;
    }

    return false;
}

void update_lines(ConsoleBuffer *buffer) {
    buffer->lines.size = 0;

    String content = console_buffer_content(buffer);
    if (content.size == 0) return;

    LineInfo info = {};
    info.line.data = content.data;
    info.line.size = 0;
    info.code_points = 0;

    LineInfo *current_line = append(buffer->lines, info);

    UTF8Iterator it = make_utf8_it(content);
    while (it.valid) {
        if (eat_new_line(&it) || (buffer->line_wrap && current_line->code_points == buffer->tile_count.x)) {
            info.line.data = content.data + it.index;
            info.line.size = 0;
            info.code_points = 0;
            current_line = append(buffer->lines, info);
        } else {
            current_line->line.size += it.utf8_size;
            current_line->code_points += 1;

            next(&it);
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
    
    V2i tile = {};
    FOR (lines, info) {
        for (UTF8Iterator it = make_utf8_it(info->line); it.valid; next(&it)) {
            buffer->display_buffer[(tile.y * buffer->tile_count.x) + tile.x].cp = it.cp;
            // TODO: Color.
            
            tile.x += 1;
            if (tile.x == buffer->tile_count.x) break;
        }
        tile.y += 1;
        tile.x  = 0;
    }
}

INTERNAL void append(ConsoleBuffer *buffer, String str) {
    s32 written = platform_write(&buffer->ring, str.data, str.size);
    if (written < str.size) {
        // TODO: This is just a hack to prevent generating invalid utf8 sequences.
        u8 *first_byte = (u8*)buffer->ring.memory + buffer->ring.alloc + buffer->ring.pos - buffer->ring.size;
        while ((*first_byte & 0xC0) == 0x80) buffer->ring.size -= 1;
    }

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

    s64 result = convert_string_to_s64(num.data, num.size);

    return result;
}

u32 const ESC_SEQUENCE_FG_COLOR = 0x01;
u32 const ESC_SEQUENCE_BG_COLOR = 0x02;
struct EscapeSequence {
    u32 kind;
    s32 args[16];
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

    str = shrink_front(str);
    s32 kind = escape_number(&str);

    if (kind == 38) {
        seq.kind = ESC_SEQUENCE_FG_COLOR;
    } else if (kind == 48) {
        seq.kind = ESC_SEQUENCE_BG_COLOR;
    } else {
        return seq;
    }

    s32 counter = 0;
    while (str.size && str.data[0] == ';') {
        str = shrink_front(str);

        s32 value = escape_number(&str);
        if (counter < 16) {
            seq.args[counter] = value;
        }

        counter += 1;
    }

    if (str.data[0] != 'm') return {};

    return seq;
}

s32 application_main(Array<String> args) {
    // NOTE: This is temporary memory.
    String starting_dir = platform_get_current_directory();

    ApplicationState state = {};
    change_path(&state.current_dir, starting_dir);
    state.running = true;

    log_to_file(Console.out);

    platform_setup_window();
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
    buffer.ring      = platform_create_ring_buffer(DefaultConsoleBufferSize);
    buffer.line_wrap = true;
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
            // TODO: preallocate a good sized buffer
            s32 const bufsize = MEGABYTES(16);
            u8 *tmp = ALLOC(default_allocator(), u8, bufsize);

            u32 pending = platform_input_available(&buffer.pec);
            while (pending) {
                s32 read = platform_read(&buffer.pec, tmp, bufsize);
                append(&buffer, {tmp, read});

                pending = platform_input_available(&buffer.pec);
            }

            DEALLOC(default_allocator(), tmp, bufsize);
        }

        if (font.is_dirty) {
            update_gpu_texture(&font_texture, font.atlas);

            font.is_dirty = false;
        }

        draw_ui(&renderer, &ui);
        platform_window_swap_buffers();

        reset_temporary_storage();
    }

    destroy_renderer(&renderer);

    return 0;
}

