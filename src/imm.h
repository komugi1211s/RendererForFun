#ifdef STB_TRUETYPE_IMPLEMENTATION
#ifndef _K_IMM_H
#define _K_IMM_H

#include "renderer.h"
#include "engine.h"

global_variable FontData *imm_font;
global_variable Input    *imm_input;

typedef struct ImmStyle {
    Color bg_color;
    Color hot_color;
    Color active_color;
} ImmStyle;

global_variable ImmStyle imm_style;

inline bool32
_imm_can_draw() {
    return imm_input && imm_font;
}

inline void
imm_set_style(ImmStyle style) {
    imm_style = style;
}

inline void
imm_begin(FontData *font_data, Input *input) {
    imm_font = font_data;
    imm_input = input;
}

inline void
imm_end() {
    imm_font = NULL;
    imm_input = NULL;
}

int32  _imm_get_text_width(char *text);

void   imm_draw_top_bar(Drawing_Buffer *buffer, int32 width, int32 height);
bool32 imm_draw_text_button(Drawing_Buffer *buffer,
                            int32 x, int32 y,
                            int32 min_width, int32 height,
                            char *text, int32 *actual_width);

void   imm_draw_text_slider(Drawing_Buffer *buffer,
                            int32 x, int32 y,
                            int32 width, int32 height,
                            char *text,
                            real32 min_value, real32 max_value,
                            real32 *output_value);

void   imm_draw_rect_category(Drawing_Buffer *buffer,
                              int32 x0, int32 y0,
                              int32 x1, int32 y1);

void   imm_draw_textinput(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1, char *buffer, size_t buffer_range);
#endif // _K_IMM_H
#endif // STB_TRUETYPE_IMPLEMENTATION
