#include <X11/keysymdef.h>

#include "logging.h"
#include "platform_window.h"
#include "platform_window.nix_x11.h"

static enum error x11_init_connection(void);
static void       x11_close_connection(void);
static enum key   x11_keycode_to_key(unsigned int keycode);
static char       x11_keypressed_to_utf8(XIC input_ctx, XKeyPressedEvent* p_keypressed_event);
static int        x11_error_handler(Display* p_display, XErrorEvent* p_error_event);

static Display* p_x11_display;
static XIM      x11_input_method;
static int      num_x11_windows;

enum error platform_window_create(int width, int height, const char* s_title, void(*p_callback_on_created)(struct platform_window*, void*), void* p_callback_on_created_user_ptr, struct platform_window* p_out_window) {
    enum error err = x11_init_connection();
    if (err != ERROR_OK) {
        return err;
    }

    Window x11_window = XCreateSimpleWindow(
        p_x11_display,
        XDefaultRootWindow(p_x11_display),   // parent
        0, 0,							     // x, y
        width ? width : 800,                 // width
        height ? height : 600,		         // height
        0,								     // border width
        0x00000000,						     // border color
        0x00000000						     // background color
    );

    XIC x11_input_ctx = XCreateIC(x11_input_method,
        XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, x11_window,
        XNFocusWindow,  x11_window,
        NULL);

    if (x11_input_ctx == NULL) {
        return ERROR_X11_CREATE_IC;
    }

    XStoreName(p_x11_display, x11_window, s_title);
    XSelectInput(p_x11_display, x11_window,
        KeyPressMask |
        KeyReleaseMask |
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask |
        ButtonMotionMask |
        FocusChangeMask |
        StructureNotifyMask |
        ExposureMask);
    XMapWindow(p_x11_display, x11_window);

    XSetICFocus(x11_input_ctx);

    *p_out_window = (struct platform_window) {
        ._p_callback_on_created = p_callback_on_created,
        ._p_callback_on_created_user_ptr = p_callback_on_created_user_ptr
    };

    struct platform_window_nix_x11_internals* p_internals = (void*)p_out_window->_bytes;
    *p_internals = (struct platform_window_nix_x11_internals) {
        .p_display = p_x11_display,
        .window = x11_window,
        .input_ctx = x11_input_ctx,
        .wm_delete_window = XInternAtom(p_x11_display, "WM_DELETE_WINDOW", 0)
    };

    XSetWMProtocols(
        p_x11_display,
        x11_window,
        &p_internals->wm_delete_window,
        1);

    ++num_x11_windows;
    
    if (p_out_window->_p_callback_on_created) {
        p_out_window->_p_callback_on_created(p_out_window, p_out_window->_p_callback_on_created_user_ptr);
    }

    return ERROR_OK;
}

void platform_window_destroy(struct platform_window* p_window) {
    if (p_window->_p_callback_on_close) {
        p_window->_p_callback_on_close(p_window, p_window->_p_callback_on_close_user_ptr);
    }
    
    struct platform_window_nix_x11_internals* p_internals = (void*)p_window->_bytes;
    XDestroyIC(p_internals->input_ctx);
    XDestroyWindow(p_internals->p_display, p_internals->window);
    
    cx_log_fmt(CX_LOG_INFO, CX_LOG_CAT_PLATFORM_WINDOW, "Window destroyed\n");

    --num_x11_windows;
    if (num_x11_windows <= 0) {
        x11_close_connection();
    }
    
    *p_window = (struct platform_window){0};
}

void platform_window_poll_events(struct platform_window* p_window) {
    struct platform_window_nix_x11_internals* p_internals = (void*)p_window->_bytes;

    while (XPending(p_internals->p_display) > 0) {
        XEvent event = {0};
        XNextEvent(p_internals->p_display, &event);

        if (event.xany.window != p_internals->window) {
            continue;
        }
        
        switch (event.type) {
            case DestroyNotify: {
                platform_window_destroy(p_window);
                return;
            }

            case ClientMessage: {
                if (event.xclient.data.l[0] == (long)p_internals->wm_delete_window) {
                    platform_window_destroy(p_window);
                }
                return;
            }

            case FocusIn: {
                if (p_window->_p_callback_on_focus_change) {
                    p_window->_p_callback_on_focus_change(
                        p_window,
                        p_window->_p_callback_on_focus_change_user_ptr,
                        1);
                }
                break;
            }

            case FocusOut: {
                if (p_window->_p_callback_on_focus_change) {
                    p_window->_p_callback_on_focus_change(
                        p_window, 
                        p_window->_p_callback_on_focus_change_user_ptr,
                        0);
                }
                break;
            }

            case ConfigureNotify: {
                if (p_window->_p_callback_on_resize) {
                    p_window->_p_callback_on_resize(
                        p_window,
                        p_window->_p_callback_on_resize_user_ptr,
                        event.xconfigure.width, 
                        event.xconfigure.height);
                }
                break;
            }

            case KeyPress: {
                if (p_window->_p_callback_on_key) {
                    p_window->_p_callback_on_key(
                        p_window,
                        p_window->_p_callback_on_key_user_ptr,
                        x11_keycode_to_key(event.xkey.keycode),
                        1);
                }

                if (p_window->_p_callback_on_char) {
                    char c = x11_keypressed_to_utf8(p_internals->input_ctx, &event.xkey);
                    if (c) {
                    p_window->_p_callback_on_char(
                        p_window,
                        p_window->_p_callback_on_char_user_ptr,
                        c);
                    }
                }

                break;
            }

            case KeyRelease: {
                if (p_window->_p_callback_on_key) {
                    p_window->_p_callback_on_key(
                        p_window,
                        p_window->_p_callback_on_key_user_ptr,
                        x11_keycode_to_key(event.xkey.keycode),
                        0);
                }
                break;
            }

            case ButtonPress:
            case ButtonRelease: {
                if (p_window->_p_callback_on_mouse_button) {
                    enum mouse_button btn = MOUSE_BUTTON__MAX;

                    switch (event.xbutton.button) {
                        case Button1: btn = MOUSE_BUTTON_left; break;
                        case Button2: btn = MOUSE_BUTTON_middle; break;
                        case Button3: btn = MOUSE_BUTTON_right; break;
                        case 8:       btn = MOUSE_BUTTON_extra1; break;
                        case 9:       btn = MOUSE_BUTTON_extra2; break;
                        default: break;
                    }

                    if (btn != MOUSE_BUTTON__MAX) {
                    p_window->_p_callback_on_mouse_button(
                        p_window, 
                        p_window->_p_callback_on_mouse_button_user_ptr, 
                        btn, 
                        event.type == ButtonPress);
                    }
                }

                if (p_window->_p_callback_on_scroll_user_ptr) {
                    switch (event.xbutton.button) {
                        case Button4: {
                            p_window->_p_callback_on_scroll(
                                p_window,
                                p_window->_p_callback_on_scroll_user_ptr,
                                -1);
                            break;
                        }
                        
                        case Button5: {
                            p_window->_p_callback_on_scroll(
                                p_window,
                                p_window->_p_callback_on_scroll_user_ptr,
                                1);
                            break;
                        }

                        case 6: /* Horizontal scroll */ break;
                        case 7: /* Horizontal scroll */ break;

                        default: break;
                    }
                }
                break;
            }

            case MotionNotify: {
                p_window->_mouse_pos[0] = event.xmotion.x;
                p_window->_mouse_pos[1] = event.xmotion.y;
                if (p_window->_p_callback_on_mouse_move) {
                    const int delta_x = p_window->_mouse_pos[0] - p_window->_mouse_pos_old[0];
                    const int delta_y = p_window->_mouse_pos[1] - p_window->_mouse_pos_old[1];
                    p_window->_p_callback_on_mouse_move(
                        p_window,
                        p_window->_p_callback_on_mouse_move_user_ptr,
                        delta_x, delta_y);
                }
                break;                
            }

            case Expose: {
                if (p_window->_p_callback_on_resize) {
                    unsigned int width, height;
                    platform_window_size(p_window, &width, &height);
                    p_window->_p_callback_on_resize(
                        p_window,
                        p_window->_p_callback_on_resize_user_ptr,
                        width, 
                        height);
                }
                break;
            }
        }
    }

    p_window->_mouse_pos_old[0] = p_window->_mouse_pos[0];
    p_window->_mouse_pos_old[1] = p_window->_mouse_pos[1];
}

int platform_window_is_open(const struct platform_window* p_window) {
    const struct platform_window_nix_x11_internals* p_internals = (const void*)p_window->_bytes;

    if (p_internals->p_display == 0 || p_internals->window == 0) {
        return 0;
    }

    XWindowAttributes window_attribs;
    return XGetWindowAttributes(p_internals->p_display, p_internals->window, &window_attribs) != BadWindow;
}

void platform_window_size(const struct platform_window* p_window, unsigned int* p_out_width, unsigned int* p_out_height) {
    const struct platform_window_nix_x11_internals* p_internals = (const void*)p_window->_bytes;

    *p_out_width =
    *p_out_height = 0;

    if (p_internals->p_display == 0 || p_internals->window == 0) {
        return;
    }

    Window win;
    int x, y;
    unsigned int b, d;

    (void)XGetGeometry(
        p_internals->p_display,
        p_internals->window,
        &win,
        &x,
        &y,
        p_out_width,
        p_out_height,
        &b,
        &d);
}

enum error x11_init_connection(void) {
    if (p_x11_display) {
        return ERROR_OK;
    }

    p_x11_display = XOpenDisplay(NULL);

    if (!p_x11_display) {
        return ERROR_X11_OPEN_DISPLAY;
    }

    (void)XSetLocaleModifiers("");

    x11_input_method = XOpenIM(p_x11_display, 0, 0, 0);
    if(!x11_input_method){
        (void)XSetLocaleModifiers("@im=none");
        x11_input_method = XOpenIM(p_x11_display, 0, 0, 0);
    }

    if (!x11_input_method) {
        return ERROR_X11_OPEN_IM;
    }

    (void)XSetErrorHandler(x11_error_handler);

    cx_log_fmt(CX_LOG_INFO, CX_LOG_CAT_PLATFORM_WINDOW, "Connection to X server established\n");

    return ERROR_OK;
}

void x11_close_connection(void) {
    XCloseIM(x11_input_method);
    XCloseDisplay(p_x11_display);

    x11_input_method = 0;
    p_x11_display = 0;

    cx_log_fmt(CX_LOG_INFO, CX_LOG_CAT_PLATFORM_WINDOW, "Connection to X server closed\n");
}

enum key x11_keycode_to_key(unsigned int keycode) {
    CX_DBG_LOG_FMT(CX_LOG_CAT_PLATFORM_WINDOW, "keycode=%u\n", keycode);

    switch (keycode) {
        case 10: return KEY_1;
        case 11: return KEY_2;
        case 12: return KEY_3;
        case 13: return KEY_4;
        case 14: return KEY_5;
        case 15: return KEY_6;
        case 16: return KEY_7;
        case 17: return KEY_8;
        case 18: return KEY_9;
        case 19: return KEY_0;
        case 24: return KEY_q;
        case 25: return KEY_w;
        case 26: return KEY_e;
        case 27: return KEY_r;
        case 28: return KEY_t;
        case 29: return KEY_y;
        case 30: return KEY_u;
        case 31: return KEY_i;
        case 32: return KEY_o;
        case 33: return KEY_p;
        case 37: return KEY_ctrl_left;
        case 38: return KEY_a;
        case 39: return KEY_s;
        case 40: return KEY_d;
        case 41: return KEY_f;
        case 42: return KEY_g;
        case 43: return KEY_h;
        case 44: return KEY_j;
        case 45: return KEY_k;
        case 46: return KEY_l;
        case 50: return KEY_shift_left;
        case 52: return KEY_z;
        case 53: return KEY_x;
        case 54: return KEY_c;
        case 55: return KEY_v;
        case 56: return KEY_b;
        case 57: return KEY_n;
        case 58: return KEY_m;
        case 65: return KEY_space;
        default: return KEY_unknown;
    }
}

char x11_keypressed_to_utf8(XIC input_ctx, XKeyPressedEvent* p_keypressed_event) {
    char sym_buf[32];
    KeySym sym;
    Status status;
    int sym_buf_len = Xutf8LookupString(input_ctx, p_keypressed_event, sym_buf, sizeof(sym_buf), &sym, &status);

    if(status == XLookupKeySym || status == XBufferOverflow){
        return 0;
    }

    if (sym_buf_len != 1) {
        return 0;
    }

    CX_DBG_LOG_FMT(CX_LOG_CAT_PLATFORM_WINDOW, "character='%c'\n", sym_buf[0]);

    return sym_buf[0];
}

int x11_error_handler(Display* p_display, XErrorEvent* p_error_event) {
    char text_buf[512];
    XGetErrorText(p_display, p_error_event->error_code, text_buf, sizeof(text_buf) - 1);
    
    cx_log_fmt(CX_LOG_ERROR, CX_LOG_CAT_PLATFORM_WINDOW, "Error: %s\n", text_buf);

    return 0;
}
