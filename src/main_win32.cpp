
// NOTE(fuzzy): this is here just for the sake of convenience.
// I'll replace this to something else eventually.
// #define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
// #include <cimgui.h>
#include <string.h>
#include <math.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC
#include "general.h"
#include "3rdparty.h"

#include "profile.cpp"
#include "maths.cpp"
#include "model.cpp"
#include "renderer.cpp"

typedef struct Input {
    bool32 forward, backward, left, right;
    bool32 debug_menu;
    real32 current_mouse_x, current_mouse_y;
    real32 prev_mouse_x,    prev_mouse_y;

    bool32 left_click, right_click;
} Input;

typedef struct UI_Layout {
    FontData *data;
    int32 x, y;

    UI_Layout(FontData *d, int32 x0, int32 y0)
    : data(d), x(x0), y(y0) {}

    void category(Drawing_Buffer *buffer, char *name) {
        draw_text(buffer, data, x, y, name);
        y += 20;
        x += 40;
    }

    void category_end() {
        x -= 40;
    }

    void text(Drawing_Buffer *buffer, char *text) {
        draw_text(buffer, data, x, y, text);
        y += 20;
    }
} UI_Layout;

global_variable bool32         running = 1;
global_variable BITMAPINFO     bitmap_info;
global_variable Drawing_Buffer drawing_buffer;
global_variable Input          input;

void render_buffer(HDC context, RECT *client_rect) {
    uint32 *current_buffer = drawing_buffer.is_drawing_first ? drawing_buffer.first_buffer : drawing_buffer.second_buffer;
    int32 client_width = client_rect->right - client_rect->left;
    int32 client_height = client_rect->bottom - client_rect->top;

    if (!(StretchDIBits(context,
          0, 0,
          drawing_buffer.window_width, drawing_buffer.window_height,
          0, 0,
          drawing_buffer.window_width, drawing_buffer.window_height,
          (void *)current_buffer, &bitmap_info, DIB_RGB_COLORS, SRCCOPY)))
    {
        int32 b = GetLastError();
        BREAK
    }
}

LRESULT CALLBACK my_window_proc(HWND handle, uint32 message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;
    /*
    if (!igGetCurrentContext()) {
        return DefWindowProc(handle, message, wparam, lparam);
    }

    ImGuiIO *io = igGetIO();
    */

    switch(message) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        {
            input.left_click = 1;
        } break;

        case WM_LBUTTONUP:
        {
            input.left_click = 0;
        } break;

        case WM_CLOSE:
        case WM_DESTROY:
        {
            running = 0;
        } break;

        case WM_MOUSEMOVE:
        {
            real32 x_pos = (real32)GET_X_LPARAM(lparam);
            real32 y_pos = (real32)GET_Y_LPARAM(lparam);

            input.current_mouse_x = x_pos;
            input.current_mouse_y = y_pos;
        } break;
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            bool32 key_pressed = message == WM_KEYDOWN;
            switch(wparam) {
                case VK_F1:
                    if (key_pressed) input.debug_menu = !input.debug_menu;
                    break;

                case 'W':
                    input.forward = key_pressed;
                    break;

                case 'S':
                    input.backward = key_pressed;
                    break;

                case 'A':
                    input.left = key_pressed;
                    break;

                case 'D':
                    input.right = key_pressed;
                    break;
                
                default:
                    break;
            }
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;

            HDC context = BeginPaint(handle, &paint);
            RECT window_rect;
            GetClientRect(handle, &window_rect);
            render_buffer(context, &window_rect);

            EndPaint(handle, &paint);
        } break;

        default:
            return DefWindowProc(handle, message, wparam, lparam);
    }
    return result;
}

bool32 load_simple_texture(char *tex_name, Texture *texture) {
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

bool32 load_simple_model(char *model_name, Model *model) {
    FILE *file;
    if ((file = fopen(model_name, "rb"))) {
        fseek(file, 0, SEEK_END);
        size_t file_length = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *buffer = (char *)VirtualAlloc(0, file_length, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        model->vertices = 
            (fVector3 *)VirtualAlloc(0, file_length * sizeof(fVector3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        model->texcoords = 
            (fVector3 *)VirtualAlloc(0, file_length * sizeof(fVector3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        model->indexes =
            (ModelIndex *)VirtualAlloc(0, file_length * sizeof(ModelIndex), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        ASSERT(buffer || model->vertices || model->indexes || model->texcoords);
        fread(buffer, 1, file_length, file);
        fclose(file);

        bool32 parsed = parse_obj(buffer, file_length, model);
        VirtualFree(buffer, 0, MEM_RELEASE);

        if(!parsed) {
            VirtualFree(model->vertices,  0, MEM_RELEASE);
            VirtualFree(model->texcoords, 0,  MEM_RELEASE);
            VirtualFree(model->indexes,   0,  MEM_RELEASE);

            TRACE("Failed to Load model.");
            return 0;
        }

    } else {
        TRACE("Failed to Open the model file.");
        return 0;
    }

    return 1;
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
    vsnprintf(bucket, EACH_STRING_SIZE,
              message, list);
    va_end(list);

    return bucket;
}

void update_camera_with_input(Camera *camera, Input *input, real32 dt) {
    if (!(input->forward && input->backward)) {
        fVector3 move_towards = fmul_fv3_fscalar(camera->target, 2.0 * dt);

        if (input->forward) {
            camera->position = fadd_fv3(camera->position, move_towards);
        }

        if (input->backward) {
            camera->position = fsub_fv3(camera->position, move_towards);
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

    if (!input->debug_menu) {
        real32 dx, dy;
        dx = input->current_mouse_x - input->prev_mouse_x;
        dy = input->current_mouse_y - input->prev_mouse_y;

        camera->yaw   += (dx * 0.5);
        camera->pitch -= (dy * 0.5);

        camera->pitch = MATHS_MAX(-89.0, MATHS_MIN(89.0, camera->pitch));

        real32 rad_yaw, rad_pitch;
        rad_yaw = MATHS_DEG2RAD(camera->yaw);
        rad_pitch = MATHS_DEG2RAD(camera->pitch);

        camera->target.x = cosf(rad_yaw) * cosf(rad_pitch);
        camera->target.y = sinf(rad_pitch);
        camera->target.z = sinf(rad_yaw) * cosf(rad_pitch);
        camera->target = fnormalize_fv3(camera->target);
    }
}

void draw_debug_menu(Drawing_Buffer *buffer, Input *input, FontData *font_data, Camera *camera, Model *model, real32 ms_per_frame, real32 fps, uint64 cycle_elapsed) {
    real32 y0 = 0.0;
    real32 top_bar_y = 30.0;

    Color box_color = rgba(0.3, 0.3, 0.3, 0.2);
    Color box_color_selected = rgba(0.0, 1.0, 1.0, 1.0);
    draw_filled_rectangle(buffer, fVec3(0.0, 0.0, 0.0), fVec3(buffer->window_width-1, top_bar_y, 0.0), box_color);

    real32 x_button_size = 200.0;
    int32 button_margin = 5.0;

    if (input->current_mouse_y > 0.0 && input->current_mouse_y < top_bar_y) {
        if (input->current_mouse_x > 0.0 && input->current_mouse_x < x_button_size) {
            if (input->left_click) {
                input->left_click = 0;
                camera->position = fVec3(0.0, 0.0, 0.0);
                camera->target   = fVec3(0.0, 0.0, 1.0);
                camera->up       = fVec3(0.0, 1.0, 0.0);

                camera->pitch = 0;
                camera->yaw = 0;
            }
            char *selected_title = "[ Reset Camera ]";
            int32 text_width = get_text_width(font_data, selected_title) + button_margin + button_margin;
            draw_filled_rectangle(buffer, fVec3((real32)button_margin, 0.0, 0.0), fVec3((real32)text_width, top_bar_y, 0.0), box_color_selected);
            draw_text(buffer, font_data, 0.0, 1.0, selected_title);
        } else {
            char *selected_title = " [Reset Camera] ";
            int32 text_width = get_text_width(font_data, selected_title) + button_margin + button_margin;
            draw_text(buffer, font_data, 0.0, 1.0, selected_title);
            draw_filled_rectangle(buffer, fVec3((real32)button_margin, 0.0, 0.0), fVec3((real32)text_width, top_bar_y, 0.0), box_color);
        }
    } else {
        char *selected_title = " [Reset Camera] ";
        int32 text_width = get_text_width(font_data, selected_title) + button_margin + button_margin;
        draw_text(buffer, font_data, 0.0, 1.0, selected_title);
        draw_filled_rectangle(buffer, fVec3((real32)button_margin, 0.0, 0.0), fVec3((real32)text_width, top_bar_y, 0.0), box_color);
    }

    y0 += top_bar_y;

    UI_Layout layout(font_data, 10, y0 + 5);
    layout.category(buffer, (char *)"Perf: ");
    layout.text(buffer, format("MpF: %lf MpF, FPS: %lf, Cycle: %lu \n", ms_per_frame, fps, cycle_elapsed));
    for (int32 i = 0; i < profile_info_count; ++i) {
        if (!profile_info[i].is_started) {
            layout.text(buffer, format("[%3d]%s, Cycle: %lu", i, profile_info[i].name, profile_info[i].cycle_elapsed));
        }
    }
    layout.category_end();

    layout.category(buffer, (char *)"Camera: ");
    layout.text(buffer, format("Pitch: [%f], Yaw: [%f]", camera->pitch, camera->yaw));
    layout.category_end();

    layout.category(buffer, (char *)"Model: ");
    layout.text(buffer, format("Vertex   count: [%lu]", model->num_vertices));
    layout.text(buffer, format("Texcoord count: [%lu]", model->num_texcoords));
    layout.text(buffer, format("Index    count: [%lu]", model->num_indexes));
    layout.category_end();
}


int32 WINAPI
WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, int obsolete) {
    int32 window_width = 1600;
    int32 window_height = 900;

    FILE *fp;
    AllocConsole();
    freopen_s(&fp, "CONOUT$", "w", stdout);

    char *class_name = "MyWindow";
    WNDCLASS window_class = {0};
    window_class.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    window_class.lpfnWndProc = my_window_proc;
    window_class.hInstance = instance;
    window_class.lpszMenuName = "None";
    window_class.lpszClassName = class_name;

    if (RegisterClassA(&window_class)) {
        HWND window = CreateWindowEx(0, class_name, "MyRenderer",
                                     WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     window_width, window_height,
                                     NULL, NULL, instance, NULL);
        if (window) {
            bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
            bitmap_info.bmiHeader.biWidth = window_width;
            bitmap_info.bmiHeader.biHeight = -window_height;
            bitmap_info.bmiHeader.biPlanes = 1;
            bitmap_info.bmiHeader.biBitCount = 32;
            bitmap_info.bmiHeader.biCompression = BI_RGB;

            drawing_buffer.window_width = window_width;
            drawing_buffer.window_height = window_height;
            drawing_buffer.depth = 32;

            FontData font_data = {0};
            if (!load_simple_font("inconsolata.ttf", &font_data, window_height)) {
                TRACE("STB FONT LOAD FAILED.");
                return 0;
            }

            Model model = {0};
            if (!load_simple_model("african_head.obj", &model)) {
                return 0;
            }

            Texture texture = {0};
            if (!load_simple_texture("african_head.tga", &texture)) {
                return 0;
            }

            Color bg = rgb_opaque(0.0, 0.2, 0.2);

            // 一つのアロケーションに全部叩き込む
            size_t color_buffer_size = (window_width * window_height * sizeof(uint32));
            size_t z_buffer_size     = (window_width * window_height * sizeof(real32));

            char *allocation = (char *)VirtualAlloc(0, color_buffer_size * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            drawing_buffer.first_buffer = (uint32 *)allocation;
            drawing_buffer.second_buffer = (uint32 *)(allocation + color_buffer_size);
            drawing_buffer.z_buffer = (real32 *)VirtualAlloc(0, z_buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            ASSERT(drawing_buffer.first_buffer && drawing_buffer.z_buffer);
            MSG message;

            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            uint64 time_frequency = freq.QuadPart;

            LARGE_INTEGER start_time, end_time;
            QueryPerformanceCounter(&start_time);

            uint64 start_cycle, end_cycle;
            uint64 time_elapsed, cycle_elapsed;
            real64 ms_per_frame, fps;
            
            Camera camera = {0};
            camera.position = fVec3(0.0, 0.0, 0.0);
            camera.target   = fVec3(0.0, 0.0, 1.0);
            camera.up       = fVec3(0.0, 1.0, 0.0);

            camera.fov = 90.0f;
            camera.aspect_ratio = 16.0 / 9.0;
            camera.z_far = 10000.0;
            camera.z_near = -100.0;

            start_cycle = __rdtsc();

            real32 delta_time = 0.016; // delta time 60fps
            while(running) {
                clear_buffer(&drawing_buffer, bg);

                while(PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                HDC context = GetDC(window);
                RECT client_rect;
                GetClientRect(window, &client_rect);

                Color color = rgb_opaque(1.0, 1.0, 1.0);
                update_camera_with_input(&camera, &input, delta_time);

                clear_buffer(&drawing_buffer, bg);
                draw_model(&drawing_buffer, &model, &camera, &texture);

                if (input.debug_menu) {
                    draw_debug_menu(&drawing_buffer, &input, &font_data, &camera, &model,
                                    ms_per_frame, fps, cycle_elapsed);
                }


                render_buffer(context, &client_rect);
                swap_buffer(&drawing_buffer);
                ReleaseDC(window, context);

                input.prev_mouse_x = input.current_mouse_x;
                input.prev_mouse_y = input.current_mouse_y;

                QueryPerformanceCounter(&end_time);
                end_cycle = __rdtsc();
                time_elapsed  = end_time.QuadPart - start_time.QuadPart;
                cycle_elapsed = end_cycle - start_cycle;
                ms_per_frame = (1000.0f * (real64)time_elapsed) / (real64)time_frequency;
                fps = (real64)time_frequency / (real64)time_elapsed;

                start_time  = end_time;
                start_cycle = end_cycle;
            }

            stbi_image_free(texture.data);
            VirtualFree(model.vertices,  0, MEM_RELEASE);
            VirtualFree(model.texcoords, 0,  MEM_RELEASE);
            VirtualFree(model.indexes,   0,  MEM_RELEASE);
            VirtualFree(drawing_buffer.first_buffer, 0, MEM_RELEASE);
            VirtualFree(drawing_buffer.z_buffer, 0, MEM_RELEASE);
        } else {
            TRACE("CreateWindowEx failed.");
            BREAK
        }
    } else {
        TRACE("RegisterClass failed.");
        return 0;
    }

    fclose(fp);
    FreeConsole();
    return 0;
}
