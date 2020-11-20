#include "imm.h"

int32 _imm_get_text_width(char *text) {
    if (!_imm_can_draw()) return 0;

    real32 x = 0;
    real32 char_scale = imm_font->char_scale;

    for (char *t = text; (*t && *t != '\n'); ++t) {
        char chara = *t;
        int32 advance, lsb;
        stbtt_GetCodepointHMetrics(&imm_font->font_info, chara,
                                   &advance, &lsb);

        x += advance * char_scale;
    }
    return (int32)ceil(x);
}


void imm_draw_top_bar(Drawing_Buffer *buffer, int32 width, int32 height) {
    if (!_imm_can_draw()) return;
    draw_filled_rectangle(buffer, 0, 0, width, height, imm_style.bg_color);
}

bool32 imm_draw_text_button(Drawing_Buffer *buffer,
                            int32 x, int32 y,
                            int32 min_width, int32 height,
                            char *text, int32 *actual_width)
{
    if (!_imm_can_draw()) return 0;
    int32 width;

    int32 text_width = _imm_get_text_width(text);
    if (text_width < min_width) {
        width = min_width;
    } else {
        width = text_width;
    }

    int32 padding = (width / 2);
    int32 txt_x = (width + padding - text_width) / 2;

    int32 x0 = x;
    int32 x1 = (x + width + padding);
    int32 y0 = y;
    int32 y1 = y + height;

    bool32 within_bounds = (x0 < imm_input->mouse.x && imm_input->mouse.x < x1
                            && y0 < imm_input->mouse.y && imm_input->mouse.y < y1);
    bool32 pressed = 0;
    Color  color = imm_style.bg_color;
    if (within_bounds) {
        color = imm_style.hot_color;
        if (imm_input->mouse.left) {
            color = imm_style.active_color;
        } else if (imm_input->prev_mouse.left) {
            pressed = 1;
        }
    }

    draw_filled_rectangle(buffer, x0, y0, x1, y1, color);
    draw_text(buffer, imm_font, x0 + txt_x, y0, text);

    if (actual_width) *actual_width = (width + padding);
    return pressed;
}

void   imm_draw_text_slider(Drawing_Buffer *buffer,
                            int32 x, int32 y,
                            int32 width, int32 height,
                            char *text,
                            real32 min_value, real32 max_value,
                            real32 *output_value)
{
    if (!_imm_can_draw()) return;

    int32 padding = (width / 2);
    int32 text_width = _imm_get_text_width(text);

    int32 txt_x = (width + padding - text_width) / 2;

    int32 x0 = x;
    int32 x1 = (x + width + padding);
    int32 y0 = y;
    int32 y1 = y + height;

    bool32 within_bounds = (x0    < imm_input->mouse.x && imm_input->mouse.x < x1
                            && y0 < imm_input->mouse.y && imm_input->mouse.y < y1);

    Color  color = imm_style.bg_color;
    if (within_bounds) {
        color = imm_style.hot_color;
        if (imm_input->mouse.left) {
            color = imm_style.active_color;
            real32 position = imm_input->mouse.x;
            real32 pos_t = (position - x0) / (x1 - x0);
            *output_value = min_value + (pos_t * max_value);
        }
    }

    real32 percentage  = (*output_value - min_value) / (max_value - min_value);
    int32 x_slider_pos = (x0 * (1.0 - percentage)) + (x1 * percentage);

    draw_filled_rectangle(buffer, x0, y0, x1, y1, color);
    draw_text(buffer, imm_font, x0 + txt_x, y0, text);
    draw_filled_rectangle(buffer, x_slider_pos - 1, y0, x_slider_pos + 1, y1, rgb_opaque(0.8, 0.8, 0.8));
    return;
}

void imm_draw_rect_category(Drawing_Buffer *buffer, int32 x0, int32 y0, int32 x1, int32 y1) { // Basically for separator / category.
    if (!_imm_can_draw()) return;
    draw_filled_rectangle(buffer, x0, y0, x1, y1, imm_style.bg_color);
}


