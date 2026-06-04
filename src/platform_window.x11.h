#ifndef PLATFORM_WINDOW_NIX_X11_H
#define PLATFORM_WINDOW_NIX_X11_H

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct platform_window_nix_x11_internals {
	Display*     p_display;
	Window       window;
	XIC          input_ctx;
	Colormap     cmap;
	Atom         wm_delete_window;
	GLXFBConfig  fbconfig;
	XVisualInfo* p_visual_info;
	Screen*      p_screen;
};

#endif
