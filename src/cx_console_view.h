#ifndef CX_CONSOLE_VIEW_H
#define CX_CONSOLE_VIEW_H

#include <stdint.h>

struct cx_console;
struct cx_font_render_data;
struct cx_gfx_framebuffer;

void cx_console_view_draw(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_render_data,
	const struct cx_gfx_framebuffer* p_fb,
	uint32_t fb_width, uint32_t fb_height,
	const float* p_projection_matrix,
	const float* p_view_matrix);

#endif
