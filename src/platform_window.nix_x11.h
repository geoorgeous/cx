#ifndef _H__PLATFORM_WINDOW_NIX_X11
#define _H__PLATFORM_WINDOW_NIX_X11

#include <X11/Xlib.h>

struct platform_window_nix_x11_internals {
    Display* p_display;
    Window   window;
    XIC      input_ctx;
    Atom     wm_delete_window;
    Screen*  p_screen;
};

#endif
