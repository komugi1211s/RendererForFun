#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC

#include "general.h"
#include "3rdparty.h"

#include "profile.c"
#include "maths.c"
#include "model.c"
#include "renderer.c"

global_variable Atom wm_protocols;
global_variable Atom wm_delete_window;

void update_xwindow_with_drawbuffer(Display *display, Window window, GC context, XColor *bg_color, Drawing_Buffer *buffer) {
    XSetForeground(display, context, bg_color->pixel);
    uint32 *drawing_buffer = buffer->is_drawing_first ? buffer->first_buffer : buffer->second_buffer;

    XImage image;
    image.width  = buffer->window_width;
    image.height = buffer->window_height;
    image.depth  = 24;
    image.bits_per_pixel = 32;
    image.bytes_per_line = buffer->window_width * sizeof(uint32);
    image.bitmap_pad = 4;
    image.bitmap_bit_order = MSBFirst;
    image.format = ZPixmap;
    image.xoffset = 0;
    image.byte_order = LSBFirst;
    image.data = (char *)drawing_buffer;

    XPutImage(display, window, context, &image, 0, 0, 0, 0, buffer->window_width, buffer->window_height);
    XFlush(display);
}

global_variable uint8 ttf_buffer[1 << 25];
bool32 load_simple_font(char *font_name, stbtt_fontinfo *font_info) {
    FILE *font = NULL;
    if ((font = fopen(font_name, "rb"))) {
        fread(ttf_buffer, 1, 1 << 25, font);
        fclose(font);

        stbtt_InitFont(font_info, ttf_buffer, 0);
        return 1;
    } else {
        return 0;
    }
}

bool32 load_simple_model(char *model_name, Model *model) {
    FILE *file;
    if ((file = fopen(model_name, "rb"))) {
        fseek(file, 0, SEEK_END);
        size_t file_length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer    = malloc(file_length + 1);
        model->vertices = malloc(file_length * sizeof(fVector3));
        model->vert_idx = malloc(file_length * sizeof(iVector3));

        ASSERT(buffer || model->vertices || model->vert_idx);
        fread(buffer, 1, file_length, file);
        fclose(file);

        buffer[file_length] = '\0';

        if(!parse_obj(buffer, file_length, model)) {
            free(buffer);
            free(model->vertices);
            free(model->vert_idx);

            TRACE("Failed to Load model.");
            return 0;
        }
    } else {
        TRACE("Failed to Open the model file.");
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    Display *display = XOpenDisplay(0);
    if (display) {
        wm_protocols = XInternAtom(display, "WM_PROTOCOLS", 0);
        wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);

        int32 start_x = 0;
        int32 start_y = 0;
        int32 window_width = 1600;
        int32 window_height = 900;
        int32 border_size = 0;

        int32 desktop_window = DefaultRootWindow(display);
        int32 default_screen = DefaultScreen(display);

        int32 screen_bit_depth = 24;
        XVisualInfo vis_info = {};
        int32 match = XMatchVisualInfo(display, default_screen, screen_bit_depth, TrueColor, &vis_info);
        if (match) {
            XSetWindowAttributes window_attr;
            window_attr.background_pixel = 0;
            window_attr.colormap = XCreateColormap(display, desktop_window,
                                                   vis_info.visual, AllocNone);

            window_attr.event_mask = StructureNotifyMask | ExposureMask;
            uint64 attr_mask = CWBackPixel | CWColormap | CWEventMask;
            TRACE("Visual info Depth: %d", vis_info.depth);

            Window my_window = XCreateWindow(display,      desktop_window,
                                             start_x,      start_y,
                                             window_width, window_height,
                                             border_size,  vis_info.depth,
                                             InputOutput,  vis_info.visual,
                                             attr_mask,    &window_attr);

            if (my_window) {
                XStoreName(display, my_window, "Window Title");
                XSetWMProtocols(display, my_window, &wm_delete_window, 1);
                {
                    XSizeHints hint = {};
                    hint.flags = PMinSize | PMaxSize;
                    hint.min_width = hint.max_width = window_width;
                    hint.min_height = hint.max_height = window_height;
                    XSetWMNormalHints(display, my_window, &hint);
                }
                GC context = XCreateGC(display, my_window, 0, 0);
                XColor approx_bg_color, true_bg_color;

                Colormap colormap = DefaultColormap(display, default_screen);
                if (!XAllocNamedColor(display, colormap, "black", &approx_bg_color, &true_bg_color)) {
                    TRACE("Failed to Allocate Color.");
                    BREAK
                }

                FontData font_data = {0};
                if (!load_simple_font("inconsolata.ttf", &font_data.font_info)) {
                    TRACE("STB FONT LOAD FAILED.");
                    return 0;
                }

                font_data.char_scale = stbtt_ScaleForPixelHeight(&font_data.font_info, window_height / 40);
                stbtt_GetFontVMetrics(&font_data.font_info, &font_data.ascent, &font_data.descent, &font_data.line_gap);
                font_data.base_line = font_data.ascent * font_data.char_scale;

                Model model = {0};
                if (!load_simple_model("african_head.obj", &model)) {
                    return 0;
                }

                XMapWindow(display, my_window);
                XFlush(display);

                Drawing_Buffer buffer = {0};
                buffer.window_width   = window_width;
                buffer.window_height  = window_height;
                buffer.depth          = screen_bit_depth;

                buffer.first_buffer  = malloc(window_width * window_height * sizeof(uint32));
                buffer.second_buffer = malloc(window_width * window_height * sizeof(uint32));
                buffer.z_buffer      = malloc(window_width * window_height * sizeof(real32));

                // MEMO: yanked this value from SDL_GetPerformanceFrequency.
                // SDL_GetPerformanceFrequency returns 1000000000 if you can use CLOCK_MONOTONIC_RAW.
                uint64 time_frequency = 1000000000;
                uint64 start_time, start_cycle;

                struct timespec _timespec_now;
                clock_gettime(CLOCK_MONOTONIC_RAW, &_timespec_now);
                start_time = (_timespec_now.tv_sec * time_frequency) + _timespec_now.tv_nsec;
                start_cycle = __rdtsc();

                bool32 running = 1;

                size_t string_message_temp_buffer_size = 1024;
                char string_message_temp_buffer[1024] = {0};
                while (running) {
                    clock_gettime(CLOCK_MONOTONIC_RAW, &_timespec_now);
                    start_time = (_timespec_now.tv_sec * time_frequency) + _timespec_now.tv_nsec;

                    XEvent event = {};
                    while(XPending(display) > 0) {
                        TRACE("Getting Event.");
                        XNextEvent(display, &event);
                        switch(event.type) {
                            case Expose:
                            {
                                XFlush(display);
                            } break;

                            case DestroyNotify:
                            {
                                XDestroyWindowEvent *destroy_ev = (XDestroyWindowEvent *) &event;
                                if (destroy_ev->window == my_window) running = 0;
                            } break;

                            case ClientMessage:
                            {
                                XClientMessageEvent *cli_ev = (XClientMessageEvent *) &event;
                                if (cli_ev->message_type == wm_protocols) {
                                    if (cli_ev->data.l[0] == wm_delete_window) {
                                        running = 0;
                                    }
                                }
                            } break;
                        }
                    }

                    Color bg = rgb_opaque(0.0, 0.0, 0.0);
                    Color c = rgb_opaque(1.0, 1.0, 1.0);

                    clear_buffer(&buffer, bg);
                    draw_model(&buffer, &model, c);

                    for (int32 i = 0; i < profile_info_count; ++i) {
                        if (!profile_info[i].is_started) {
                            memset(string_message_temp_buffer, 0,
                                    string_message_temp_buffer_size);

                            snprintf(string_message_temp_buffer,
                                    string_message_temp_buffer_size,
                                    "[%d] Name: %s, Cycle: %10lu", i, profile_info[i].name, profile_info[i].cycle_elapsed);
                            draw_text(&buffer, &font_data, 10, 40 + (20 * i), string_message_temp_buffer);
                        }
                    }

                    // NOTE(fuzzy): Lofted this from Handmade Hero.
                    clock_gettime(CLOCK_MONOTONIC_RAW, &_timespec_now);
                    uint64 end_time = (_timespec_now.tv_sec * time_frequency) + _timespec_now.tv_nsec;
                    uint64 end_cycle = __rdtsc();

                    uint64 time_elapsed = end_time - start_time;
                    uint64 cycle_elapsed = end_cycle - start_cycle;

                    real64 ms_per_frame = (1000.0f * (real64)time_elapsed) / (real64)time_frequency;
                    real64 fps = (real64)time_frequency / (real64)time_elapsed;

                    memset(string_message_temp_buffer, 0,
                            string_message_temp_buffer_size);

                    snprintf(string_message_temp_buffer,
                            string_message_temp_buffer_size,
                            "MpF: %lf MpF, FPS: %lf, Cycle: %lu \n", ms_per_frame, fps, cycle_elapsed);
                    draw_text(&buffer, &font_data, 10, 20, string_message_temp_buffer);
                    update_xwindow_with_drawbuffer(display, my_window, context, &approx_bg_color, &buffer);
                    swap_buffer(&buffer);

                    start_time  = end_time;
                    start_cycle = end_cycle;
                }
                XCloseDisplay(display);
            } else {
                TRACE("Failed to Open Window.");
                BREAK
            }
        } else {
            TRACE("Appropriate Visualinfo does not exist.");
            BREAK
        }
    } else {
        TRACE("Failed to Open XDisplay.");
        BREAK
    }
    return 0;
}
