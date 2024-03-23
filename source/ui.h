#pragma once

#include "definitions.h"
#include "thermal.h"
#include "renderer.h"


enum UILayoutKind {
    UI_LAYOUT_NONE,
    UI_LAYOUT_WIDGET,
    UI_LAYOUT_ROW,
};

enum UILayoutRowKind {
    UI_LAYOUT_EVEN,
    UI_LAYOUT_EXACT
};

struct UILayout {
    UILayoutKind kind;

    V2 dim;

    r32 total_width;
    r32 used_width;
    s32 columns;
    s32 column;

    s32 rows;
    s32 row;
};

struct UIRect {
    r32 x;
    r32 y;
    r32 w;
    r32 h;
};

struct UIHittest {
    void *id;

    UIRect region;
};

enum UITaskKind {
    UI_TASK_REGULAR,
    UI_TASK_TEXT,
};
struct UITask {
    UITaskKind kind;
    GPUTexture *texture;

    s32 offset;
    s32 count;
};

struct UIWindow {
    struct UIState *ui;

    void *id;
    b32 active;

    V2 pos;
    V2 dim;

    V2 next_widget_pos;

    String title;
    UILayout layout;

    DArray<UITask> tasks;
    DArray<UIHittest>  hittests;
};

struct UIVertex {
    V3 pos;
    V2 uv;
    V4 color;
};

struct UITextVertex {
    V3 pos;
    V2 uv;
    u32 fg;
    u32 bg;
};

struct UIPersistentData {
    void *id;

    b32 active;
};

struct UIState {
    void *hover;
    void *focus;
    void *last_clicked;

    V2 draw_region;
    r32 margin;

    UserInput input;
    struct Font *font;
    GPUTexture *font_texture;

    DArray<UIWindow> windows;
    UIWindow *current_window;
    UIWindow  default_window;

    DArray<UIVertex> vertices;
    DArray<UITextVertex> text_vertices;

    DArray<UIPersistentData> widget_data;

    GPUBuffer text_vertex_buffer;
    GPUShader text_shader;
};


void init_ui(UIState *ui, Font *font);
void destroy_ui(UIState *ui);

void begin_frame(UIState *ui, V2 window_size, UserInput *input);
void end_frame(UIState *ui);

void row_layout(UIState *ui, r32 height, s32 columns, UILayoutRowKind kind);
void column_width(UIState *ui, r32 width);

enum {
    UI_TEXT_TOP_LEFT,
    UI_TEXT_CENTER_LEFT,
    UI_TEXT_CENTER_CENTER,
};
void draw_text(UIState *ui, UIRect rect, Color color, String text, u32 align);
void text_label(UIState *ui, String text);
b32 button(UIState *ui, void *id, String text, bool active = true);
b32 button(UIState *ui, String text, bool active = true);

b32 expandable_window_seperator(UIState *ui, void *id, String label);

b32 console_buffer_view(UIState *ui, void *id, ConsoleBuffer *buffer);


void draw_ui(Renderer *renderer, UIState *ui);

