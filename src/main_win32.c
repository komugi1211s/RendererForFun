
// NOTE(fuzzy): this is here just for the sake of convenience.
// I'll replace this to something else eventually.
// #define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
// #include <cimgui.h>
#include <string.h>
#include <math.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC
#include "general.h"
#include "3rdparty.h"
#include "profile.c"

#include "maths.c"
#include "model.c"
#include "renderer.c"

global_variable bool32 running = 1;
global_variable BITMAPINFO bitmap_info;
global_variable Drawing_Buffer drawing_buffer;

void render_buffer(HDC context, RECT *client_rect) {
    uint32 *current_buffer = drawing_buffer.is_drawing_first ? drawing_buffer.first_buffer : drawing_buffer.second_buffer;
    int32 client_width = client_rect->right - client_rect->left;
    int32 client_height = client_rect->bottom - client_rect->top;

    int32 a;
    if (!(a = StretchDIBits(context,
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
        /*
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        {
            if (!GetCapture()) SetCapture(handle);
            io->MouseDown[0] = true;
        } break;

        case WM_LBUTTONUP:
        {
            io->MouseDown[0] = false;
        } break;
        */

        case WM_CLOSE:
        case WM_DESTROY:
        {
            running = 0;
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

        char *buffer =
            VirtualAlloc(0, file_length, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        model->vertices =
            VirtualAlloc(0, file_length * sizeof(fVector3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        model->texcoords =
            VirtualAlloc(0, file_length * sizeof(fVector3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        model->indexes =
            VirtualAlloc(0, file_length * sizeof(ModelIndex), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

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

            Texture texture = {0};
            if (!load_simple_texture("african_head.tga", &texture)) {
                return 0;
            }

            Color bg = rgb_opaque(0.0, 0.0, 0.0);
            size buffer_size = window_height * window_width * sizeof(uint32);
            drawing_buffer.first_buffer =
                VirtualAlloc(0, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            drawing_buffer.second_buffer =
                VirtualAlloc(0, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            size z_buffer_size = window_height * window_width * sizeof(real32);
            drawing_buffer.z_buffer =
                VirtualAlloc(0, z_buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            ASSERT(drawing_buffer.first_buffer && drawing_buffer.second_buffer);
            MSG message;

            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            uint64 time_frequency = freq.QuadPart;

            LARGE_INTEGER start_time, end_time;
            QueryPerformanceCounter(&start_time);

            uint64 start_cycle, end_cycle;
            start_cycle = __rdtsc();

            Camera camera;
            camera.position = fVec3(0.0, 0.0, -1.0);
            camera.target   = fVec3(0.0, 0.0, 1.0);
            camera.up       = fVec3(0.0, 1.0, 0.0);

            size_t string_message_temp_buffer_size = 1024;
            char string_message_temp_buffer[1024] = {0};

            while(running) {
                while(PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                HDC context = GetDC(window);
                RECT client_rect;
                GetClientRect(window, &client_rect);

                Color color = rgb_opaque(1.0, 1.0, 1.0);


                clear_buffer(&drawing_buffer, bg);
                draw_model(&drawing_buffer, &model, &texture);

                for (int32 i = 0; i < profile_info_count; ++i) {
                    if (!profile_info[i].is_started) {
                        memset(string_message_temp_buffer, 0,
                                string_message_temp_buffer_size);

                        snprintf(string_message_temp_buffer,
                                string_message_temp_buffer_size,
                                "[%d] Name: %s, Cycle: %10llu", i, profile_info[i].name, profile_info[i].cycle_elapsed);
                        draw_text(&drawing_buffer, &font_data, 10, 40 + (20 * i), string_message_temp_buffer);
                    }
                }

                QueryPerformanceCounter(&end_time);
                end_cycle = __rdtsc();
                uint64 time_elapsed  = end_time.QuadPart - start_time.QuadPart;
                uint64 cycle_elapsed = end_cycle - start_cycle;
                real64 ms_per_frame = (1000.0f * (real64)time_elapsed) / (real64)time_frequency;
                real64 fps = (real64)time_frequency / (real64)time_elapsed;
                memset(string_message_temp_buffer, 0,
                        string_message_temp_buffer_size);

                snprintf(string_message_temp_buffer,
                        string_message_temp_buffer_size,
                        "MpF: %lf MpF, FPS: %lf, Cycle: %llu \n", ms_per_frame, fps, cycle_elapsed);
                draw_text(&drawing_buffer, &font_data, 10, 20, string_message_temp_buffer);

                render_buffer(context, &client_rect);
                swap_buffer(&drawing_buffer);
                ReleaseDC(window, context);
                start_time  = end_time;
                start_cycle = end_cycle;
            }

            stbi_image_free(texture.data);
            VirtualFree(model.vertices,  0, MEM_RELEASE);
            VirtualFree(model.texcoords, 0,  MEM_RELEASE);
            VirtualFree(model.indexes,   0,  MEM_RELEASE);
            VirtualFree(drawing_buffer.first_buffer, 0, MEM_RELEASE);
            VirtualFree(drawing_buffer.second_buffer, 0, MEM_RELEASE);
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
