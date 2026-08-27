#include <ctype.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/keysymdef.h>

#include "cx_dbg.h"
#include "cx_logging.h"
#include "cx_error.h"
#include "platform_window.h"
#include "input.h"
#include "platform_window.x11.h"

static enum cx_error x11_init_connection(void);
static void          x11_close_connection(void);
static enum cx_key   x11_keycode_to_key(unsigned int keycode);
static unsigned int  x11_mods_to_input_mods(unsigned int mods);
static char          x11_keypressed_to_utf8(XIC input_ctx, XKeyPressedEvent* p_keypressed_event);
static int           x11_error_handler(Display* p_display, XErrorEvent* p_error_event);

static Display* p_x11_display;
static XIM      x11_input_method;
static int      num_x11_windows;

enum cx_error platform_window_create(
	uint32_t width, uint32_t height,
	const char* s_title,
	void(*f_callback_on_created)(struct platform_window*, void*),
	void* f_callback_on_created_user_ptr,
	struct platform_window* p_out_window) {

	enum cx_error err = x11_init_connection();
	if (err != CX_ERROR_none) {
		return err;
	}

	if (!width) {
		width = 800;
	}

	if (!height) {
		height = 600;
	}

	const int default_screen = DefaultScreen(p_x11_display);
	const Window root_window = XDefaultRootWindow(p_x11_display);

	Window x11_window = 0;

	const int fb_attribs[] = {
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER,  True,
		GLX_RED_SIZE,      8,
		GLX_GREEN_SIZE,    8,
		GLX_BLUE_SIZE,     8,
		GLX_ALPHA_SIZE,    8,
		GLX_DEPTH_SIZE,    24,
		GLX_STENCIL_SIZE,  8,
		None
	};

	GLXFBConfig chosen_fbconfig = 0;
	XVisualInfo* p_chosen_visual_info = 0;

	int fbconfigs_count;
	GLXFBConfig* p_fbconfigs = glXChooseFBConfig(p_x11_display, default_screen, fb_attribs, &fbconfigs_count);

	for (int i = 0; i < fbconfigs_count; ++i) {
		XVisualInfo* p_visual_info = glXGetVisualFromFBConfig(p_x11_display, p_fbconfigs[i]);
		
		if (!p_visual_info) {
			continue;
		}

		chosen_fbconfig = p_fbconfigs[i];
		p_chosen_visual_info = p_visual_info;
		break;
	}

	XFree(p_fbconfigs);

	if (!p_chosen_visual_info) {
		CX_DBG(CX_LOG(ERROR, PLATFORM_WINDOW, "Failed to find required visual info\n"));
		return CX_ERROR_api_glx;
	}

	const Colormap x11_cmap = XCreateColormap(p_x11_display, root_window, p_chosen_visual_info->visual, AllocNone);

	XSetWindowAttributes attribs = {
		.colormap = x11_cmap,
		.background_pixel = None,
		.border_pixmap = None,
		.event_mask = 
			KeyPressMask |
			KeyReleaseMask |
			ButtonPressMask |
			ButtonReleaseMask |
			PointerMotionMask |
			ButtonMotionMask |
			FocusChangeMask |
			StructureNotifyMask |
			ExposureMask
	};

	const unsigned long attrib_mask =
		CWColormap |
		CWBorderPixel |
		CWEventMask;

	x11_window = XCreateWindow(
		p_x11_display,
		root_window,
		0, 0,
		width, height,
		0,
		p_chosen_visual_info->depth,
		InputOutput,
		p_chosen_visual_info->visual,
		attrib_mask,
		&attribs);

	if (!x11_window) {
		CX_DBG(CX_LOG(ERROR, PLATFORM_WINDOW, "Failed to create platform window\n"));

		return CX_ERROR_api_x11;
	}

	XIC x11_input_ctx = XCreateIC(x11_input_method,
		XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, x11_window,
		XNFocusWindow,  x11_window,
		NULL);

	if (!x11_input_ctx) {
		CX_DBG(CX_LOG(ERROR, PLATFORM_WINDOW, "Failed to create platform input context\n"));
		return CX_ERROR_api_x11;
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
		.f_callback_on_created_ = f_callback_on_created,
		.p_callback_on_created_user_ptr_ = f_callback_on_created_user_ptr
	};

	struct platform_window_nix_x11_internals* p_internals = (void*)p_out_window->internals_.bytes_;
	*p_internals = (struct platform_window_nix_x11_internals) {
		.p_display = p_x11_display,
		.window = x11_window,
		.cmap = x11_cmap,
		.input_ctx = x11_input_ctx,
		.wm_delete_window = XInternAtom(p_x11_display, "WM_DELETE_WINDOW", 0),
		.fbconfig = chosen_fbconfig,
		.p_visual_info = p_chosen_visual_info
	};

	XSetWMProtocols(
		p_x11_display,
		x11_window,
		&p_internals->wm_delete_window,
		1);

	++num_x11_windows;
	
	if (p_out_window->f_callback_on_created_) {
		p_out_window->f_callback_on_created_(p_out_window, p_out_window->p_callback_on_created_user_ptr_);
	}

	return CX_ERROR_none;
}

void platform_window_destroy(struct platform_window* p_window) {
	if (p_window->f_callback_on_close_) {
		p_window->f_callback_on_close_(p_window, p_window->p_callback_on_close_user_ptr_);
	}
	
	struct platform_window_nix_x11_internals* p_internals = (void*)p_window->internals_.bytes_;
	XFree(p_internals->p_visual_info);
	XDestroyIC(p_internals->input_ctx);
	XDestroyWindow(p_internals->p_display, p_internals->window);
	XFreeColormap(p_internals->p_display, p_internals->cmap);
	
	CX_LOG(INFO, PLATFORM_WINDOW, "Window destroyed\n");

	--num_x11_windows;
	if (num_x11_windows <= 0) {
		x11_close_connection();
	}
	
	*p_window = (struct platform_window){0};
}

void platform_window_poll_events(struct platform_window* p_window) {
	struct platform_window_nix_x11_internals* p_internals = (void*)p_window->internals_.bytes_;

	while (p_internals->p_display && XPending(p_internals->p_display) > 0) {
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
				if (p_window->f_callback_on_focus_change_) {
					p_window->f_callback_on_focus_change_(
						p_window,
						p_window->p_callback_on_focus_change_user_ptr_,
						1);
				}
				break;
			}

			case FocusOut: {
				if (p_window->f_callback_on_focus_change_) {
					p_window->f_callback_on_focus_change_(
						p_window, 
						p_window->p_callback_on_focus_change_user_ptr_,
						0);
				}
				break;
			}

			case ConfigureNotify: {
				if (p_window->f_callback_on_resize_) {
					p_window->f_callback_on_resize_(
						p_window,
						p_window->p_callback_on_resize_user_ptr_,
						(uint32_t)event.xconfigure.width, 
						(uint32_t)event.xconfigure.height);
				}
				break;
			}

			case KeyPress: {
				if (p_window->f_callback_on_key_) {
					p_window->f_callback_on_key_(
						p_window,
						p_window->p_callback_on_key_user_ptr_,
						x11_keycode_to_key(event.xkey.keycode),
						1,
						x11_mods_to_input_mods(event.xkey.state));
				}

				if (p_window->f_callback_on_char_) {
					const char c = x11_keypressed_to_utf8(p_internals->input_ctx, &event.xkey);
					if (c) {
					p_window->f_callback_on_char_(
						p_window,
						p_window->p_callback_on_char_user_ptr_,
						(unsigned int)c);
					}
				}

				break;
			}

			case KeyRelease: {
				if (p_window->f_callback_on_key_) {
					p_window->f_callback_on_key_(
						p_window,
						p_window->p_callback_on_key_user_ptr_,
						x11_keycode_to_key(event.xkey.keycode),
						0,
						x11_mods_to_input_mods(event.xkey.state));
				}
				break;
			}

			case ButtonPress:
			case ButtonRelease: {
				const unsigned int input_mods = x11_mods_to_input_mods(event.xbutton.state);

				if (p_window->f_callback_on_mouse_button_) {
					enum mouse_button btn = MOUSE_BUTTON_MAX_;

					switch (event.xbutton.button) {
						case Button1: btn = MOUSE_BUTTON_left; break;
						case Button2: btn = MOUSE_BUTTON_middle; break;
						case Button3: btn = MOUSE_BUTTON_right; break;
						case 8:       btn = MOUSE_BUTTON_extra1; break;
						case 9:       btn = MOUSE_BUTTON_extra2; break;
						default: break;
					}

					if (btn != MOUSE_BUTTON_MAX_) {
					p_window->f_callback_on_mouse_button_(
						p_window, 
						p_window->p_callback_on_mouse_button_user_ptr_, 
						btn, 
						event.type == ButtonPress,
						input_mods);
					}
				}

				if (p_window->p_callback_on_scroll_user_ptr_) {
					switch (event.xbutton.button) {
						case Button4: {
							p_window->f_callback_on_scroll_(
								p_window,
								p_window->p_callback_on_scroll_user_ptr_,
								-1,
								input_mods);
							break;
						}
						
						case Button5: {
							p_window->f_callback_on_scroll_(
								p_window,
								p_window->p_callback_on_scroll_user_ptr_,
								1,
								input_mods);
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
				p_window->mouse_pos_[0] = event.xmotion.x;
				p_window->mouse_pos_[1] = event.xmotion.y;
				if (p_window->f_callback_on_mouse_move_) {
					const int delta_x = p_window->mouse_pos_[0] - p_window->mouse_pos_old_[0];
					const int delta_y = p_window->mouse_pos_[1] - p_window->mouse_pos_old_[1];
					p_window->f_callback_on_mouse_move_(
						p_window,
						p_window->p_callback_on_mouse_move_user_ptr_,
						delta_x, delta_y,
						x11_mods_to_input_mods(event.xmotion.state));
				}
				break;                
			}

			case Expose: {
				if (p_window->f_callback_on_resize_) {
					unsigned int width, height;
					platform_window_size(p_window, &width, &height);
					p_window->f_callback_on_resize_(
						p_window,
						p_window->p_callback_on_resize_user_ptr_,
						width, 
						height);
				}
				break;
			}

			default: break;
		}
	}

	p_window->mouse_pos_old_[0] = p_window->mouse_pos_[0];
	p_window->mouse_pos_old_[1] = p_window->mouse_pos_[1];
}

int platform_window_is_open(const struct platform_window* p_window) {
	const struct platform_window_nix_x11_internals* p_internals = (const void*)p_window->internals_.bytes_;

	if (p_internals->p_display == 0 || p_internals->window == 0) {
		return 0;
	}

	XWindowAttributes window_attribs;
	return XGetWindowAttributes(p_internals->p_display, p_internals->window, &window_attribs) != BadWindow;
}

void platform_window_size(
	const struct platform_window* p_window,
	uint32_t* p_out_width, uint32_t* p_out_height) {
	
	const struct platform_window_nix_x11_internals* p_internals = (const void*)p_window->internals_.bytes_;

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
		&x, &y,
		p_out_width, p_out_height,
		&b, &d);
}

enum cx_error x11_init_connection(void) {
	if (p_x11_display) {
		return CX_ERROR_none;
	}

	p_x11_display = XOpenDisplay(NULL);

	if (!p_x11_display) {
		return CX_ERROR_api_x11;
	}

	(void)XSetLocaleModifiers("");

	x11_input_method = XOpenIM(p_x11_display, 0, 0, 0);
	if(!x11_input_method){
		(void)XSetLocaleModifiers("@im=none");
		x11_input_method = XOpenIM(p_x11_display, 0, 0, 0);
	}

	if (!x11_input_method) {
		return CX_ERROR_api_x11;
	}

	(void)XSetErrorHandler(x11_error_handler);

	CX_LOG(INFO, PLATFORM_WINDOW, "Connection to X server established\n");

	return CX_ERROR_none;
}

void x11_close_connection(void) {
	XCloseIM(x11_input_method);
	XCloseDisplay(p_x11_display);

	x11_input_method = 0;
	p_x11_display = 0;

	CX_LOG(INFO, PLATFORM_WINDOW, "Connection to X server closed\n");
}

enum cx_key x11_keycode_to_key(unsigned int keycode) {
	CX_DBG(CX_LOG_FMT(TRACE, PLATFORM_WINDOW, "keycode=%u\n", keycode));

	switch (keycode) {
		case  9:  return CX_KEY_escape;
		case 10:  return CX_KEY_1;
		case 11:  return CX_KEY_2;
		case 12:  return CX_KEY_3;
		case 13:  return CX_KEY_4;
		case 14:  return CX_KEY_5;
		case 15:  return CX_KEY_6;
		case 16:  return CX_KEY_7;
		case 17:  return CX_KEY_8;
		case 18:  return CX_KEY_9;
		case 19:  return CX_KEY_0;
		case 22:  return CX_KEY_backspace;
		case 23:  return CX_KEY_tab;
		case 24:  return CX_KEY_q;
		case 25:  return CX_KEY_w;
		case 26:  return CX_KEY_e;
		case 27:  return CX_KEY_r;
		case 28:  return CX_KEY_t;
		case 29:  return CX_KEY_y;
		case 30:  return CX_KEY_u;
		case 31:  return CX_KEY_i;
		case 32:  return CX_KEY_o;
		case 33:  return CX_KEY_p;
		case 36:  return CX_KEY_enter;
		case 37:  return CX_KEY_ctrl_left;
		case 38:  return CX_KEY_a;
		case 39:  return CX_KEY_s;
		case 40:  return CX_KEY_d;
		case 41:  return CX_KEY_f;
		case 42:  return CX_KEY_g;
		case 43:  return CX_KEY_h;
		case 44:  return CX_KEY_j;
		case 45:  return CX_KEY_k;
		case 46:  return CX_KEY_l;
		case 49:  return CX_KEY_grave;
		case 50:  return CX_KEY_shift_left;
		case 52:  return CX_KEY_z;
		case 53:  return CX_KEY_x;
		case 54:  return CX_KEY_c;
		case 55:  return CX_KEY_v;
		case 56:  return CX_KEY_b;
		case 57:  return CX_KEY_n;
		case 58:  return CX_KEY_m;
		case 65:  return CX_KEY_space;
		case 67:  return CX_KEY_f1;
		case 68:  return CX_KEY_f2;
		case 69:  return CX_KEY_f3;
		case 70:  return CX_KEY_f4;
		case 71:  return CX_KEY_f5;
		case 72:  return CX_KEY_f6;
		case 73:  return CX_KEY_f7;
		case 74:  return CX_KEY_f8;
		case 75:  return CX_KEY_f9;
		case 76:  return CX_KEY_f10;
		case 95:  return CX_KEY_f11;
		case 96:  return CX_KEY_f12;
		case 110: return CX_KEY_home;
		case 111: return CX_KEY_up;
		case 113: return CX_KEY_left;
		case 114: return CX_KEY_right;
		case 115: return CX_KEY_end;
		case 116: return CX_KEY_down;
		case 119: return CX_KEY_delete;
		default:  return CX_KEY_unknown;
	}
}

unsigned int x11_mods_to_input_mods(unsigned int mods) {
	return
		(!!(mods & ControlMask) * INPUT_MOD_ctrl) |
		(!!(mods & ShiftMask) * INPUT_MOD_shift) |
		(!!(mods & Mod1Mask) * INPUT_MOD_1) |
		(!!(mods & Mod2Mask) * INPUT_MOD_2) |
		(!!(mods & Mod3Mask) * INPUT_MOD_3) |
		(!!(mods & Mod4Mask) * INPUT_MOD_4) |
		(!!(mods & Mod5Mask) * INPUT_MOD_5);
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

CX_DBG(
	if (!iscntrl(sym_buf[0])) {
		CX_LOG_FMT(TRACE, PLATFORM_WINDOW, "character='%c'\n", sym_buf[0]);
	}
);

	return sym_buf[0];
}

int x11_error_handler(Display* p_display, XErrorEvent* p_error_event) {
	char text_buf[512];
	XGetErrorText(p_display, p_error_event->error_code, text_buf, sizeof(text_buf) - 1);
	
	CX_LOG_FMT(ERROR, PLATFORM_WINDOW, "Error: %s\n", text_buf);

	return 0;
}
