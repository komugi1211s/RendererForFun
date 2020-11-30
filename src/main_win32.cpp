
#include <string.h>
#include <math.h>
#include <intrin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>


#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBTT_STATIC

#include "general.h"
#include "3rdparty.h"

#include "maths.cpp"
#include "memory.cpp"
#include "engine.cpp"
#include "asset.cpp"
#include "renderer.cpp"
#include "imm.cpp"

global_variable bool32         running = 1;
global_variable BITMAPINFO     bitmap_info;
global_variable ScreenBuffer   drawing_buffer;
global_variable Input          input;

void render_buffer(HDC context, RECT *client_rect) {
    PROFILE_FUNC;
    uint32 *current_buffer = drawing_buffer.back_buffer;
    int32 client_width  = client_rect->right  - client_rect->left;
    int32 client_height = client_rect->bottom - client_rect->top;

    StretchDIBits(context,
                  0, 0,
                  drawing_buffer.window_width, drawing_buffer.window_height,
                  0, 0,
                  drawing_buffer.window_width, drawing_buffer.window_height,
                  (void *)current_buffer, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK my_window_proc(HWND handle, uint32 message, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    switch(message) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        {
            input.mouse.left = 1;
        } break;

        case WM_LBUTTONUP:
        {
            input.mouse.left = 0;
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

            input.mouse.x = x_pos;
            input.mouse.y = y_pos;
        } break;
        case WM_KEYUP:
        case WM_KEYDOWN:
        {
            bool32 key_pressed = message == WM_KEYDOWN;
            switch(wparam) {
                case VK_F1:
                if (key_pressed) input.debug_menu_key = !input.debug_menu_key;
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
    PROFILE_FUNC;
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

    if (!input->debug_menu_key) {
        real32 dx, dy;
        dx = input->mouse.x - input->prev_mouse.x;
        dy = input->mouse.y - input->prev_mouse.y;

        real32 sensitivity = 100.0 * dt;
        camera->yaw   += (dx * sensitivity);
        camera->pitch += (dy * sensitivity);

        camera->pitch = FP_CLAMP(camera->pitch, -89.0, 89.0);

        real32 rad_yaw, rad_pitch;
        rad_yaw = FP_DEG2RAD(camera->yaw);
        rad_pitch = FP_DEG2RAD(camera->pitch);

        camera->target.x = cosf(rad_yaw) * cosf(rad_pitch);
        camera->target.y = sinf(rad_pitch);
        camera->target.z = sinf(rad_yaw) * cosf(rad_pitch);
        camera->target = fnormalize_fv3(camera->target);
    }
}

void draw_debug_info(ScreenBuffer *buffer,
                     Input        *input,
                     FontData     *font_data,
                     Camera       *camera,
                     Model        *model,
                     real32 ms_per_frame, real32 fps, uint64 cycle_elapsed)
{
    Color box_color = rgba(0.1, 0.1, 0.1, 0.2);
    int32 x_window_size = 500;
    int32 y_window_size = 300;
    int32 x_window_pos = 5;
    int32 y_window_pos = 50;

    draw_filled_rectangle(buffer, x_window_pos, y_window_pos, x_window_pos + x_window_size, y_window_pos + y_window_size, box_color);

    int32 x = 20;
    int32 y = 60;
    for (int32 i = 0; i < profile_info_count; ++i) {
        draw_text(buffer, font_data, x, y, format("[%d] %s | %lu cy", i, profile_info[i].name, profile_info[i].cycle_elapsed));
        y += 20;
    }
    y += 10;
    draw_text(buffer, font_data, x, y, format("%5.2f MpF(%5.2f FPS), %lu Cycle Elapsed", ms_per_frame, fps, cycle_elapsed));
    y += 20;
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

void default_camera(Camera *cam) {
    cam->position     = fVec3(0.0, 0.0, 2.0);
    cam->target       = fVec3(0.0, 0.0, 1.0);
    cam->up           = fVec3(0.0, 1.0, 0.0);
    cam->fov          = 90.0f;
    cam->aspect_ratio = 16.0 / 9.0;
    cam->z_far        = 1000.0;
    cam->z_near       = 0.01;
    cam->yaw          = 90.0f;
    cam->pitch        = 0.0f;
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

void *platform_allocate_memory(size_t memory_size) {
    void *memory = VirtualAlloc(0, memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (!memory) {
        TRACE("MEMORY ALLOCATION FAILED: SIZE REQUESTED %zu", memory_size);
        return NULL;
    }

    return memory;
}

void platform_free_memory(void *ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

FileObject
platform_open_and_read_entire_file(char *file_name) {
    FileObject result = {0};
    FILE *fp = fopen(file_name, "rb");

    if (!fp) return result;

    /*
     *
     * NOTE(fuzzy):
     * !fseek でもいいが、 == 0 としたほうが
     * 「意図した値である事をチェックしている」感がでるのでこのまま
     *
     * 早期リターンを使用してもいいが、いちいちfclose(fp);をかくのも冗長なので
     * ifを連ねることにする
     *
     * */
    if(fseek(fp, 0, SEEK_END) == 0) {
        size_t file_length = ftell(fp);

        if (file_length > 0 && (fseek(fp, 0, SEEK_SET) == 0)) {
            char *memory = (char *)platform_allocate_memory(file_length + 1);

            if (memory) {
                fread(memory, file_length, 1, fp);
                fclose(fp);

                memory[file_length] = '\0';
                result.opened  = 1;
                result.content = memory;
                result.size    = file_length;

                return result;
            }
        }
    }

    fclose(fp);
    return result;
}

int32 WINAPI
WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, int obsolete) {
    int32 window_width = 1600;
    int32 window_height = 900;
    if (__argc <= 1) {
        MessageBox(NULL,
                   "コマンドラインから main.exe [OBJファイル名] [テクスチャ名(optional)] のように起動して下さい。",
                   NULL,
                   MB_OK);
        return 0;
    }

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

            // =======================================
            // レンダリングに必要な物全般をセットアップ
            // =======================================

            size_t core_memory_size = MEGABYTES(128);
            void   *memory = platform_allocate_memory(core_memory_size);

            if (!memory) {
                MessageBox(window, "128MBのメモリの割り当てに失敗しました。", NULL, MB_OK);
                return 0;
            }
            Engine engine;

            engine.core_memory.initialize((uint8 *)memory, core_memory_size);

            size_t asset_capacity = 32;
            engine.asset_table.models.initialize((Model *)engine.core_memory.alloc(asset_capacity * sizeof(Model)), asset_capacity);
            engine.asset_table.textures.initialize((Texture *)engine.core_memory.alloc(asset_capacity * sizeof(Texture)), asset_capacity);

            engine.platform.allocate_memory    = platform_allocate_memory;
            engine.platform.deallocate_memory  = platform_free_memory;
            engine.platform.open_and_read_file = platform_open_and_read_entire_file;

            // TODO(fuzzy): 未だ使っていない
            Property property;
            property.position = fVec3(0.0, 0.0, 0.0);
            property.rotation = fVec3(0.0, 0.0, 0.0);
            property.scale    = fVec3(1.0, 1.0, 1.0);

            ImmStyle default_style;
            default_style.bg_color = rgba(0.1, 0.1, 0.1, 0.5);
            default_style.hot_color = rgba(0.5, 0.0, 0.0, 0.5);
            default_style.active_color = rgba(0.0, 0.5, 0.0, 0.5);
            imm_set_style(default_style);

            drawing_buffer.window_width = window_width;
            drawing_buffer.window_height = window_height;
            drawing_buffer.depth = 32;

            size_t color_buffer_size = (window_width * window_height * sizeof(uint32));
            size_t z_buffer_size     = (window_width * window_height * sizeof(real32));

            drawing_buffer.visible_buffer = (uint32 *)engine.core_memory.alloc(color_buffer_size);
            drawing_buffer.back_buffer    = (uint32 *)engine.core_memory.alloc(color_buffer_size);
            drawing_buffer.z_buffer       = (real32 *)engine.core_memory.alloc(z_buffer_size);

            Camera camera;
            default_camera(&camera);

            FontData font_data = {0};
            if (!load_simple_font("inconsolata.ttf", &font_data, window_height)) {
                MessageBox(window, "inconsolata.ttfの読み込みに失敗しました。", NULL, MB_OK);
                TRACE("STB FONT LOAD FAILED.");
                return 0;
            }

            if (file_extension_matches(__argv[1], "obj")) {
                if (!load_model(&engine, __argv[1])) {
                    MessageBox(window, "指定されたモデルがロードできませんでした。", NULL, MB_OK);
                    return 0;
                }
            } else {
                MessageBox(window, "指定されたモデルがロードできませんでした。", NULL, MB_OK);
                return 0;
            }
            if(__argc >= 3) {
                if (!load_texture(&engine, __argv[2])) {
                    // NOTE(fuzzy): @MemoryLeak
                    // どの道即終了するのでfree(model->... は行わない
                    MessageBox(window, "指定されたテクスチャがロードできませんでした。", NULL, MB_OK);
                    return 0;
                }
            }
            // TODO(fuzzy): コンパイルを通すためだけにここにある
            Model model = engine.asset_table.models[0];
            Texture texture = engine.asset_table.textures[0];

            MSG message;

            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            uint64 time_frequency = freq.QuadPart;

            LARGE_INTEGER start_time, end_time;
            QueryPerformanceCounter(&start_time);

            uint64 start_cycle; // NOTE(fuzzy): while文の直前に初期化する
            uint64 time_elapsed  = 0;
            uint64 cycle_elapsed = 0;

            real64 ms_per_frame  = 0;
            real64 fps           = 0;

            Color fg_color = rgb_opaque(1.0, 1.0, 1.0);
            real32 delta_time     = 0.016;
            bool32 draw_z_buffer  = 0;
            bool32 wireframe_on   = 0;
            clear_buffer(&drawing_buffer, CLEAR_COLOR_BUFFER | CLEAR_Z_BUFFER, camera.z_far);

            start_cycle = __rdtsc();
            while(running) {
                PROFILE("Clear buffer before rendering") {
                    clear_buffer(&drawing_buffer, CLEAR_COLOR_BUFFER | CLEAR_Z_BUFFER, camera.z_far);
                }

                while(PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                if(!input.debug_menu_key) update_camera_with_input(&camera, &input, delta_time);

                if (wireframe_on) {
                    draw_wire_model(&drawing_buffer, &model, &camera, &property, fg_color);
                } else {
                    if(texture.data && model.texcoords.count > 0) {
                        draw_textured_model(&drawing_buffer, &model, &camera, &property, &texture);
                    } else {
                        draw_filled_model(&drawing_buffer, &model, &camera, &property, fg_color);
                    }
                }

                if (draw_z_buffer) {
                    DEBUG_render_z_buffer(&drawing_buffer, camera.z_far);
                }

                if (input.debug_menu_key) {
                    PROFILE("Drawing Debug Menu") {
                        draw_debug_info(&drawing_buffer, &input, &font_data, &camera, &model,
                                        ms_per_frame, fps, cycle_elapsed);
                        imm_begin(&font_data, &input);
                        imm_draw_top_bar(&drawing_buffer, drawing_buffer.window_width, 30);
                        int32 topbar_y_size = 30;
                        int32 current_x = 0;
                        int32 min_width = 120;
                        int32 actual_width = 0;
                        {
                            char *name = (char *)"Reset Camera";
                            if(imm_draw_text_button(&drawing_buffer, current_x, 0, min_width, topbar_y_size, name, &actual_width)) {
                                default_camera(&camera);
                            }
                            current_x += actual_width;
                        }
                        {
                            char *name = (char *)"Z Buffer";
                            if(imm_draw_text_button(&drawing_buffer, current_x, 0, min_width, topbar_y_size, name, &actual_width)) {
                                draw_z_buffer = !draw_z_buffer;
                                wireframe_on = 0;
                            }
                            current_x += actual_width;
                        }
                        {
                            char *name = (char *)"Wireframe";
                            if(imm_draw_text_button(&drawing_buffer, current_x, 0, min_width, topbar_y_size, name, &actual_width)) {
                                wireframe_on = !wireframe_on;
                                draw_z_buffer = 0;
                            }
                            current_x += actual_width;
                        }
                        {
                            char *name = format("FOV: %06.2f", camera.fov);
                            int32 fixed_width = 170;
                            imm_draw_text_slider(&drawing_buffer, current_x, 0, fixed_width, topbar_y_size, name, 0.0, 120.0, &camera.fov);
                            current_x += fixed_width;
                        }
                        imm_end();
                    }
                }

                HDC context = GetDC(window);
                RECT client_rect;
                GetClientRect(window, &client_rect);
                render_buffer(context, &client_rect);
                ReleaseDC(window, context);

                swap_buffer(&drawing_buffer);
                input.prev_mouse = input.mouse;

                LARGE_INTEGER end_time;
                QueryPerformanceCounter(&end_time);
                uint64 end_cycle = __rdtsc();
                time_elapsed  = end_time.QuadPart - start_time.QuadPart;
                cycle_elapsed = end_cycle - start_cycle;
                ms_per_frame = (1000.0f * (real64)time_elapsed) / (real64)time_frequency;
                fps = (real64)time_frequency / (real64)time_elapsed;

                start_time  = end_time;
                start_cycle = end_cycle;
            }

            stbi_image_free(texture.data);

            free_models(&engine);
            free_textures(&engine);
            platform_free_memory(engine.core_memory.data);
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
