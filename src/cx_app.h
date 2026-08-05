#ifndef CX_APP_H
#define CX_APP_H

#include <stdint.h>

struct cx_gfx_framebuffer;
struct platform_window;

typedef int(*cx_app_init_callback_fn)(int argc, const char** argv);
typedef void(*cx_app_update_callback_fn)(double);
typedef void(*cx_app_draw_callback_fn)(const struct cx_gfx_framebuffer*);
typedef void(*cx_app_shutdown_callback_fn)(void);

int cx_app_init(
	const char* p_name,
	uint32_t window_width,
	uint32_t window_height,
	cx_app_init_callback_fn f_init,
	int argc,
	const char** argv);
void cx_app_run(cx_app_update_callback_fn f_update, cx_app_draw_callback_fn f_draw);
void cx_app_shutdown(cx_app_shutdown_callback_fn f_shutdown);
struct platform_window* cx_app_primary_window(void);

#endif
