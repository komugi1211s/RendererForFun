#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <string.h>
#include <math.h>

#include "general.h"
#include "renderer.h"
#include "renderer.c"

global_variable Atom wm_protocols;
global_variable Atom wm_delete_window;

void update_xwindow_with_drawbuffer(Display *display, Window window, GC context, XColor *bg_color, Drawing_Buffer *buffer) {
    TRACE("Drawing.");
    XSetForeground(display, context, bg_color->pixel);
    uint32 *drawing_buffer = buffer->is_drawing_first ? buffer->first_buffer : buffer->first_buffer;

    XImage image;
    image.width  = buffer->window_width;
    image.height = buffer->window_height;
    image.depth  = 24;
    image.bits_per_pixel = 32;
    image.bytes_per_line = buffer->window_width * 4;
    image.bitmap_pad = 8;
    image.bitmap_bit_order = MSBFirst;
    image.format = ZPixmap;
    image.xoffset = 0;
    image.byte_order = MSBFirst;
    image.data = (char *)drawing_buffer;

    XPutImage(display, window, context, &image, 0, 0, 0, 0, buffer->window_width, buffer->window_height);
    XFlush(display);
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
                if (!XAllocNamedColor(display, colormap, "white", &approx_bg_color, &true_bg_color)) {
                    TRACE("Failed to Allocate Color.");
                    BREAK
                }

                XMapWindow(display, my_window);
                XFlush(display);

                Drawing_Buffer buffer;
                buffer.window_width   = window_width;
                buffer.window_height  = window_height;
                buffer.depth          = screen_bit_depth;

                buffer.first_buffer   = malloc(window_width * window_height * sizeof(uint32));
                buffer.second_buffer  = malloc(window_width * window_height * sizeof(uint32));
                buffer.is_drawing_first = 1;

                bool32 running = 1;
                while (running) {
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

                    draw_line(&buffer, 10, 20, 80, 40);
                    update_xwindow_with_drawbuffer(display, my_window, context,
                                                   &approx_bg_color, &buffer);

                    swap_buffer(&buffer);
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
