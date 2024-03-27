#pragma once

#include "definitions.h"
#include "platform.h"
#include "string2.h"
#include "io.h"
#include "font.h"


enum Key {
    KEY_TEXT      = 0x00,
    KEY_BACKSPACE = 0x08,
    KEY_RETURN    = 0x0D,
    KEY_DELETE    = 0x7F,

    KEY_LEFT_ARROW,
    KEY_RIGHT_ARROW,

    KEY_HOME,
    KEY_END,
};
struct KeyPress {
    Key key;
    u32 code_point;
};

struct MouseInput {
    V2 cursor;
    b32 lmb;
    s32 scroll;
};

u32 const KeyBufferSize = 32;
struct UserInput {
    KeyPress key_buffer[KeyBufferSize];
    u32 key_buffer_used;

    b32 alt_held;
    b32 ctrl_held;
    b32 shift_held;

    MouseInput last_mouse;
    MouseInput mouse;
};

struct ApplicationState {
    UserInput user_input;

    V2  window_size;
    r32 window_aspect_ratio;
    b32 window_size_changed;

    Path current_dir;

    String data_dir;

    b32 running;
};


u32 const PROMPT_BUFFER_SIZE = 128;
enum PromtKind {
    PROMPT_STATIC,
    PROMPT_UPDATE_EACH_COMMAND,
    // PROMPT_UPDATE_EACH_FRAME,
};
struct PromptBuffer {
    PromtKind kind;
    u32 buffer[PROMPT_BUFFER_SIZE];
    u32 buffer_used;

    String format;
};

enum {
    CONSOLE_TILE_FLAG_BOLD   = 0x01,
    CONSOLE_TILE_FLAG_ITALIC = 0x02,

    CONSOLE_TILE_FLAGS_BOLD_ITALIC = CONSOLE_TILE_FLAG_BOLD | CONSOLE_TILE_FLAG_ITALIC,
};
struct ConsoleTile {
    u32 cp;
    u32 style;
    u32 fg;
    u32 bg;
};

struct LineInfo {
    Array<ConsoleTile> line;
};

struct ConsoleBuffer {
    PlatformRingBuffer ring;
    DArray<LineInfo> lines;
    s32 scroll_offset;

    DArray<ConsoleTile> display_buffer;

    DArray<u32> command;
    s32 cursor_pos;
    Array<String32> history;
    
    PromptBuffer prompt;

    Array<u8> pipe_buffer;

    Array<ConsoleTile> conversion_buffer;

    ConsoleFont *font;

    u32 fg_color;
    u32 bg_color;

    u32 current_tile_flags;
    u32 current_fg;
    u32 current_bg;

    s32 display_start;

    b32 line_wrap;

    V2i tile_count;

    PlatformExecutionContext pec;
};


void update_lines(ConsoleBuffer *buffer);
void update_display_buffer(ConsoleBuffer *buffer);

