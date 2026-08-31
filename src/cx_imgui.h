#ifndef CX_IMGUI_H
#define CX_IMGUI_H

#include <stddef.h>
#include <stdint.h>

#include "cx_color.h"

#define CX_LOG_CAT_IMGUI "imgui"

typedef int32_t cx_imgui_id;

struct cx_imgui_vec2 {
	float x;
	float y;
};

struct cx_imgui_rect {
	struct cx_imgui_vec2 position;
	struct cx_imgui_vec2 size;
};

struct cx_font_render_data;

struct cx_imgui_theme {
	struct cx_color color_background;
	struct cx_color color_foreground;
	struct cx_color color_accent_a;
	struct cx_color color_accent_b;
	const struct cx_font_render_data* p_font;
	float           padding_x;
	float           padding_y;
	float           border_size;
	float           text_input_width;
};

struct cx_imgui_layout {
	struct cx_imgui_vec2 cursor;
	int                  b_same_line;
	float                line_height;
	float                next_width;
};

struct cx_imgui_quad {
	struct cx_imgui_rect rect;
	struct cx_color      color;
};

struct cx_imgui_text {
	struct cx_imgui_vec2 positon;
	const char* p_str;
	size_t      len;
};

struct cx_imgui {
	cx_imgui_id hot;
	cx_imgui_id active;

	size_t text_input_cursor_pos;

	struct cx_imgui_theme theme;

	struct cx_imgui_layout layout;

	struct cx_imgui_quad quads[256];
	struct cx_imgui_text texts[256];
};

void cx_imgui_begin(struct cx_imgui* p_ctx);
void cx_imgui_end(struct cx_imgui* p_ctx);

void cx_imgui_separator(struct cx_imgui* p_ctx);
void cx_imgui_text(struct cx_imgui* p_ctx, const char* s_str);
int  cx_imgui_button(struct cx_imgui* p_ctx, const char* s_str);
int  cx_imgui_text_input(struct cx_imgui* p_ctx, char* p_buf, size_t buf_len);
// dropdown
// checkbox

void  cx_imgui_same_line(struct cx_imgui* p_ctx);
float cx_imgui_get_available_width(struct cx_imgui* p_ctx);
void  cx_imgui_set_next_width(struct cx_imgui* p_ctx, float next_width);

#endif
