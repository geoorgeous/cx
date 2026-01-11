#ifndef _H__PLATFORM_WINDOW_NIX_X11
#define _H__PLATFORM_WINDOW_NIX_X11

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct platform_window_nix_x11_internals {
    Display*    p_display;
    Window      window;
    XIC         input_ctx;
    Atom        wm_delete_window;
	GLXFBConfig fbconfig;
    Screen*     p_screen;
};

#endif
