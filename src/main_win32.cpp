
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
#include "engine.h"

#include "profile.cpp"
#include "maths.cpp"
#include "model.cpp"
#include "renderer.cpp"
#include "imm.cpp"

global_variable bool32         running = 1;
global_variable BITMAPINFO     bitmap_info;
global_variable Drawing_Buffer drawing_buffer;
global_variable Input          input;

void render_buffer(HDC context, RECT *client_rect) {
    uint32 *current_buffer = drawing_buffer.is_drawing_first ? drawing_buffer.first_buffer : drawing_buffer.second_buffer;
    int32 client_width  = client_rect->right  - client_rect->left;
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

void default_camera(Camera *cam) {
    cam->position     = fVec3(0.0, 0.0, 2.0);
    cam->target       = fVec3(0.0, 0.0, 0.0);
    cam->up           = fVec3(0.0, 1.0, 0.0);
    cam->fov          = 90.0f;
    cam->aspect_ratio = 16.0 / 9.0;
    cam->z_far        = 100.0;
    cam->z_near       = 0.1;
    cam->yaw          = 90.0f;
    cam->pitch        = 0.0f;
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
            
            ImmStyle default_style;
            default_style.bg_color = rgba(0.25, 0.25, 0.25, 0.5);
            default_style.hot_color = rgba(0.50, 0.0, 0.0, 0.5);
            default_style.active_color = rgba(0.0, 0.5, 0.0, 0.5);
            
            imm_set_style(default_style);
            
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
            default_camera(&camera);
            
            start_cycle = __rdtsc();
            
            real32 delta_time = 0.016; // delta time 60fps
            bool32 wireframe_on = 0;
            bool32 draw_z_buffer = 0;
            while(running) {
                HDC context = GetDC(window);
                RECT client_rect;
                GetClientRect(window, &client_rect);

                render_buffer(context, &client_rect);
                ReleaseDC(window, context);

                clear_buffer(&drawing_buffer, CLEAR_COLOR_BUFFER | CLEAR_Z_BUFFER);
                
                while(PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                
                Color color = rgb_opaque(1.0, 1.0, 1.0);
                update_camera_with_input(&camera, &input, delta_time);
                
                if (wireframe_on) {
                    draw_wire_model(&drawing_buffer, &model, &camera, color);
                } else {
                    draw_filled_model(&drawing_buffer, &model, &camera, color);
                }
                
                if (draw_z_buffer) {
                    DEBUG_render_z_buffer(&drawing_buffer);
                }
                
                if (input.debug_menu_key) {
                    draw_debug_menu(&drawing_buffer, &input, &font_data, &camera, &model,
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
                
                swap_buffer(&drawing_buffer);
                input.prev_mouse = input.mouse;
                input.prev_mouse = input.mouse;
                
                QueryPerformanceCounter(&end_time);
                end_cycle = __rdtsc();
                time_elapsed  = end_time.QuadPart - start_time.QuadPart;
                cycle_elapsed = end_cycle - start_cycle;
                ms_per_frame = (1000.0f * (real64)time_elapsed) / (real64)time_frequency;
                fps = (real64)time_frequency / (real64)time_elapsed;
                
                start_time  = end_time;
                start_cycle = end_cycle;
            }
            
            // stbi_image_free(texture.data);
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
