#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC

#include "general.h"
#include "3rdparty.h"
#include "profile.cpp"
#include "maths.cpp"
#include "model.cpp"
#include "renderer.cpp"
#include "imm.cpp"

global_variable Atom wm_protocols;
global_variable Atom wm_delete_window;

void update_camera_with_input(Camera *camera, Input *input, real32 dt) {
    if (!(input->forward && input->backward)) {
        fVector3 move_towards = fmul_fv3_fscalar(camera->target, 2.0 * dt);

        if (input->forward) {
            camera->position = fsub_fv3(camera->position, move_towards);
        }

        if (input->backward) {
            camera->position = fadd_fv3(camera->position, move_towards);
        }
    }

    if (!(input->left && input->right)) {
        fVector3 move_towards = fnormalize_fv3(fcross_fv3(camera->target, camera->up));
        move_towards = fmul_fv3_fscalar(move_towards, 2.0 * dt);
        if (input->right) {
            camera->position = fsub_fv3(camera->position, move_towards);
        }

        if (input->left) {
            camera->position = fadd_fv3(camera->position, move_towards);
        }
    }

    real32 dx, dy;
    dx = input->mouse.x - input->prev_mouse.x;
    dy = input->mouse.y - input->prev_mouse.y;

    real32 sensitivity = 100.0 * dt;
    camera->yaw   += (dx * sensitivity);
    camera->pitch -= (dy * sensitivity);

    camera->pitch = MATHS_MAX(-89.0, MATHS_MIN(89.0, camera->pitch));

    real32 rad_yaw, rad_pitch;
    rad_yaw = MATHS_DEG2RAD(camera->yaw);
    rad_pitch = MATHS_DEG2RAD(camera->pitch);

    camera->target.x = cosf(rad_yaw) * cosf(rad_pitch);
    camera->target.y = sinf(rad_pitch);
    camera->target.z = sinf(rad_yaw) * cosf(rad_pitch);
    camera->target = fnormalize_fv3(camera->target);
}

void update_xwindow_with_drawbuffer(Display *display, Window window, GC context, Drawing_Buffer *buffer) {
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
bool32 load_simple_font(const char *font_name, FontData *font_data, int32 window_height) {
    FILE *font = NULL;
    if ((font = fopen(font_name, "rb"))) {
        fread(ttf_buffer, 1, 1 << 25, font);
        fclose(font);

        stbtt_InitFont(&font_data->font_info, ttf_buffer, 0);
        font_data->char_scale = stbtt_ScaleForPixelHeight(&font_data->font_info, window_height / 40);
        stbtt_GetFontVMetrics(&font_data->font_info, &font_data->ascent, &font_data->descent, &font_data->line_gap);
        font_data->base_line = font_data->ascent * font_data->char_scale;
        return 1;
    } else {
        return 0;
    }
}

bool32 load_simple_texture(const char *tex_name, Texture *texture) {
    texture->data = stbi_load(tex_name,
                              &texture->width,
                              &texture->height,
                              &texture->channels, 0);

    if (texture->channels != 3) {
        stbi_image_free(texture->data);
        return 0;
    }

    return !!(texture->data);
}

bool32 load_simple_model(const char *model_name, Model *model) {
    FILE *file;
    if ((file = fopen(model_name, "rb"))) {
        fseek(file, 0, SEEK_END);
        size_t file_length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer    = (char *)malloc(file_length + 1);
        model->vertices = (fVector3 *)malloc(file_length * sizeof(fVector3));
        model->texcoords = (fVector3 *)malloc(file_length * sizeof(fVector3));
        model->indexes  = (ModelIndex *)malloc(file_length * sizeof(ModelIndex));

        ASSERT(buffer || model->vertices || model->texcoords || model->indexes);
        fread(buffer, 1, file_length, file);
        fclose(file);

        buffer[file_length] = '\0';
        bool32 parsed = parse_obj(buffer, file_length, model);
        free(buffer);

        if(!parsed) {
            free(model->vertices);
            free(model->texcoords);
            free(model->indexes);

            TRACE("Failed to Load model.");
            return 0;
        }
    } else {
        TRACE("Failed to Open the model file.");
        return 0;
    }
    return 1;
}

#define EACH_STRING_SIZE 1024
#define BUCKET_SIZE 16
char *format(const char * __restrict message, ...) {
    local_persist char temp_string_bucket[BUCKET_SIZE][EACH_STRING_SIZE];
    local_persist size_t bucket_index;

    bucket_index = ((bucket_index + 1) & (BUCKET_SIZE - 1));
    char *bucket = temp_string_bucket[bucket_index];
    memset(bucket, 0, EACH_STRING_SIZE);

    va_list list;
    va_start(list, message);
    vsnprintf(bucket,  EACH_STRING_SIZE,
              message, list);
    va_end(list);

    return bucket;
}

void draw_debug_menu(Drawing_Buffer *buffer, Input *input, FontData *font_data, Camera *camera, Model *model, real32 ms_per_frame, real32 fps, uint64 cycle_elapsed) {
    Color box_color = rgba(0.1, 0.1, 0.1, 0.2);
    draw_filled_rectangle(buffer, 10, 50, 600, 300, box_color);

    int32 x = 20;
    int32 y = 60;
    for (int32 i = 0; i < profile_info_count; ++i) {
        if (!profile_info[i].is_started) {
            draw_text(buffer, font_data, x, y, format("[%d] %s | %lu cy", i, profile_info[i].name, profile_info[i].cycle_elapsed));
            y += 20;
        }
    }
    y += 10;
    draw_text(buffer, font_data, x, y, (char *)"Camera");
    x += 40;
    y += 20;
    draw_text(buffer, font_data, x, y, format("Position: [%2f, %2f, %2f]", camera->position.x, camera->position.y, camera->position.z));
    y += 20;
    draw_text(buffer, font_data, x, y, format("LookAt:   [%2f, %2f, %2f]", camera->target.x, camera->target.y, camera->target.z));
    y += 20;
    draw_text(buffer, font_data, x, y, format("Yaw, Pitch: [%2f, %2f]", camera->yaw, camera->pitch));
    y += 20;
    x -= 40;
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

            window_attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
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

                XMapWindow(display, my_window);
                XFlush(display);

                // =======================================
                // レンダリングに必要な物全般をセットアップ
                // =======================================
                FontData font_data = {0};
                if (!load_simple_font("inconsolata.ttf", &font_data, window_height)) {
                    TRACE("STB FONT LOAD FAILED.");
                    return 0;
                }

                ImmStyle default_style;
                default_style.bg_color = rgba(0.25, 0.25, 0.25, 0.5);
                default_style.hot_color = rgba(0.50, 0.0, 0.0, 0.5);
                default_style.active_color = rgba(0.0, 0.5, 0.0, 0.5);

                imm_set_style(default_style);

                Model model = {0};
                if (!load_simple_model("african_head.obj", &model)) {
                    return 0;
                }

                Texture texture = {0};
                if (!load_simple_texture("african_head.tga", &texture)) {
                    // どの道即終了するのでfree(model->... は行わない
                    return 0;
                }

                Drawing_Buffer buffer = {0};
                buffer.window_width   = window_width;
                buffer.window_height  = window_height;
                buffer.depth          = screen_bit_depth;

                // 一つのアロケーションに全部叩き込む
                size_t color_buffer_size = (window_width * window_height * sizeof(uint32));
                size_t z_buffer_size     = (window_width * window_height * sizeof(real32));
                char *main_buffer_allocation = (char *)malloc(color_buffer_size * 2);
                char *z_buffer_allocation = (char *)malloc(z_buffer_size);

                buffer.first_buffer  = (uint32 *)main_buffer_allocation;
                buffer.second_buffer = (uint32 *)(main_buffer_allocation + color_buffer_size);
                buffer.z_buffer      = (real32 *)z_buffer_allocation;

                Input input = {0};

                Camera camera = {0};
                camera.position = fVec3(0.0, 0.0, 2.0);
                camera.target   = fVec3(0.0, 0.0, 1.0);
                camera.up       = fVec3(0.0, 1.0, 0.0);
                camera.fov = 80.0f;
                camera.aspect_ratio = 16.0 / 9.0;
                camera.z_far = 10000.0;
                camera.z_near = 0.0;
                camera.yaw = 90.0f;

                // MEMO: yanked this value from SDL_GetPerformanceFrequency.
                // SDL_GetPerformanceFrequency returns 1000000000 if you can use CLOCK_MONOTONIC_RAW.
                uint64 time_frequency = 1000000000;
                uint64 start_time, start_cycle;

                struct timespec _timespec_now;
                clock_gettime(CLOCK_MONOTONIC_RAW, &_timespec_now);
                start_time = (_timespec_now.tv_sec * time_frequency) + _timespec_now.tv_nsec;
                start_cycle = __rdtsc();

                uint64 time_elapsed, cycle_elapsed;
                real64 ms_per_frame, fps;
                Color bg = rgb_opaque(0.0, 0.2, 0.2);

                bool32 running = 1;

                bool32 wireframe_on = 0;
                while (running) {
                    clear_buffer(&buffer, bg);

                    clock_gettime(CLOCK_MONOTONIC_RAW, &_timespec_now);
                    start_time = (_timespec_now.tv_sec * time_frequency) + _timespec_now.tv_nsec;
                    start_cycle = __rdtsc();

                    XEvent event = {};
                    while(XPending(display) > 0) {
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

                            case KeyPress:
                            case KeyRelease:
                            {
                                bool32 pressed = event.type == KeyPress;
                                XKeyEvent *key_ev = (XKeyEvent *)&event;

                                uint32 key_code = key_ev->keycode;
                                uint32 keysym = XkbKeycodeToKeysym(display, key_code, 0, 0);

                                switch(keysym) {
                                    case XK_a:
                                        input.left = pressed;
                                        break;
                                    case XK_d:
                                        input.right = pressed;
                                        break;
                                    case XK_s:
                                        input.backward = pressed;
                                        break;
                                    case XK_w:
                                        input.forward = pressed;
                                        break;
                                    case XK_F1:
                                        if(pressed) input.debug_menu_key = !input.debug_menu_key;
                                        break;
                                    default:
                                        break;
                                }
                            } break;

                            case MotionNotify:
                            {
                                XPointerMovedEvent *ev = (XPointerMovedEvent *)&event;
                                input.mouse.x = ev->x;
                                input.mouse.y = ev->y;
                            } break;

                            case ButtonPress:
                            case ButtonRelease:
                            {
                                XButtonEvent *ev = (XButtonEvent *)&event;
                                if (ev->button == 1) {
                                    input.mouse.left = event.type == ButtonPress;
                                }
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
                    Color c = rgb_opaque(1.0, 1.0, 1.0);

                    update_camera_with_input(&camera, &input, 0.016);
                    draw_textured_model(&buffer, &model, &camera, &texture);

                    if (input.debug_menu_key) {
                        draw_debug_menu(&buffer, &input, &font_data, &camera, &model,
                                        ms_per_frame, fps, cycle_elapsed);
                        imm_begin(&font_data, &input);
                        imm_draw_top_bar(&buffer, buffer.window_width, 30);

                        int32 topbar_y_size = 30;
                        int32 current_x = 0;
                        int32 min_width = 120;
                        int32 actual_width = 0;
                        {
                            char *name = (char *)"Reset Camera";
                            if(imm_draw_text_button(&buffer, current_x, 0, min_width, topbar_y_size, name, &actual_width)) {
                                camera.position = fVec3(0.0, 0.0, 2.0);
                                camera.target   = fVec3(0.0, 0.0, 1.0);
                                camera.up       = fVec3(0.0, 1.0, 0.0);
                                camera.fov = 90.0f;
                                camera.aspect_ratio = 16.0 / 9.0;
                                camera.z_far = 10000.0;
                                camera.z_near = 0.0;
                                camera.yaw = 90.0f;
                            }
                            current_x += actual_width;
                        }
                        {
                            char *name = (char *)"Wireframe";
                            if(imm_draw_text_button(&buffer, current_x, 0, min_width, topbar_y_size, name, &actual_width)) {
                                wireframe_on = !wireframe_on;
                            }
                            current_x += actual_width;
                        }

                        imm_end();
                    }
                    update_xwindow_with_drawbuffer(display, my_window, context, &buffer);
                    swap_buffer(&buffer);

                    input.prev_mouse = input.mouse;

                    clock_gettime(CLOCK_MONOTONIC_RAW, &_timespec_now);
                    uint64 end_time = (_timespec_now.tv_sec * time_frequency) + _timespec_now.tv_nsec;
                    uint64 end_cycle = __rdtsc();

                    time_elapsed  = end_time - start_time;
                    cycle_elapsed = end_cycle - start_cycle;

                    ms_per_frame = (1000.0f * (real64)time_elapsed) / (real64)time_frequency;
                    fps          = (real64)time_frequency / (real64)time_elapsed;

                    start_time  = end_time;
                    start_cycle = end_cycle;
                }

                free(buffer.first_buffer);
                free(buffer.z_buffer);
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
