#include <string.h>
#include <math.h>
#include <windows.h>

#include "general.h"
#include "renderer.h"
#include "renderer.c"

global_variable bool32 running = 1;
global_variable BITMAPINFO bitmap_info;
global_variable Drawing_Buffer drawing_buffer;

void render_buffer(HDC context, RECT *client_rect) {
    uint32 *current_buffer = drawing_buffer.is_drawing_first ? drawing_buffer.first_buffer : drawing_buffer.second_buffer;

    int32 client_width = client_rect->right - client_rect->left;
    int32 client_height = client_rect->bottom - client_rect->top;
    OutputDebugStringA("Rendering\n");

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
    switch(message) {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            running = 0;
        } break;

        case WM_PAINT:
        {
            OutputDebugStringA("Paintng");
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

int32 WINAPI 
WinMain(HINSTANCE instance, HINSTANCE prev_instance, char *cmd_line, int obsolete) {
    int32 window_width = 1600;
    int32 window_height = 900;

    char *class_name = "MyWindow";
    WNDCLASS window_class = {0};
    window_class.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    window_class.lpfnWndProc = my_window_proc;
    window_class.hInstance = instance;
    window_class.lpszMenuName = "None";
    window_class.lpszClassName = class_name;

    if (RegisterClassA(&window_class)) {
        HWND window = CreateWindowEx(0, class_name, "MyRenderer",
                                     (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME) | WS_VISIBLE,
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
            drawing_buffer.is_drawing_first = 1;
            drawing_buffer.depth = 32;

            size buffer_size = window_height * window_width * sizeof(uint32);
            drawing_buffer.first_buffer  =
                VirtualAlloc(0, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            drawing_buffer.second_buffer =
                VirtualAlloc(0, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            ASSERT(drawing_buffer.first_buffer && drawing_buffer.second_buffer);
            MSG message;
            while(running) {
                while(PeekMessage(&message, window, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                HDC context = GetDC(window);
                RECT client_rect;
                GetClientRect(window, &client_rect);
                draw_line(&drawing_buffer, 10, 20, 800, 100);
                render_buffer(context, &client_rect);
                swap_buffer(&drawing_buffer);
                ReleaseDC(window, context);
            }
            VirtualFree(drawing_buffer.first_buffer, 0, MEM_RELEASE);
            VirtualFree(drawing_buffer.second_buffer, 0, MEM_RELEASE);
        } else {
            TRACE("CreateWindowEx failed.");
            BREAK
        }
    } else {
        TRACE("RegisterClass failed.");
        return 0;
    }
    return 0;
}
