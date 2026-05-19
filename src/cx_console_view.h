#ifndef CX_CONSOLE_VIEW_H
#define CX_CONSOLE_VIEW_H

struct cx_console;
struct cx_font_render_data;

void cx_console_view_draw(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_render_data,
	const float* p_projection_matrix,
	const float* p_view_matrix);

#endif
