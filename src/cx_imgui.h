#ifndef CX_IMGUI_H
#define CX_IMGUI_H

#include <stddef.h>
#include <stdint.h>

#include "cx_color.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_shader_program_interface.h"
#include "cx_render_draw_command.h"

#define CX_LOG_CAT_IMGUI "imgui"

#define CX_IMGUI_MAX_QUADS 256
#define CX_IMGUI_MAX_TEXTS 256
#define CX_IMGUI_MAX_DRAW_COMMANDS (CX_IMGUI_MAX_QUADS + CX_IMGUI_MAX_TEXTS)

typedef uint32_t cx_imgui_id;

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
	struct cx_color colors[5];
	const struct cx_font_render_data* p_font;
	float           padding_x;
	float           padding_y;
	float           border_size;
	float           text_input_width;
	float           dropdown_width;
};

struct cx_imgui_layout {
	struct cx_imgui_vec2 cursor;
	int                  b_same_line;
	float                line_height;
	float                next_width;
	int                  b_scissor;
	struct cx_imgui_rect scissor_region;
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

	uint16_t width;
	uint16_t height;

	struct cx_gfx_shader_program_input_texture input_texture;

	struct cx_gfx_shader_program_input_block input_blocks[CX_IMGUI_MAX_DRAW_COMMANDS];

	struct cx_render_draw_command draw_commands[CX_IMGUI_MAX_DRAW_COMMANDS];

	float draw_command_vertex_matrices[CX_IMGUI_MAX_DRAW_COMMANDS][16];

	float draw_command_colors[CX_IMGUI_MAX_DRAW_COMMANDS][4];

	struct cx_gfx_mesh text_meshes[CX_IMGUI_MAX_TEXTS];

	uint16_t num_quad_draw_commands;
	uint16_t num_text_draw_commands;
};

void cx_imgui_begin(struct cx_imgui* p_ctx, uint16_t width, uint16_t height);
void cx_imgui_end(struct cx_imgui* p_ctx);

void cx_imgui_begin_window(struct cx_imgui* p_ctx, const char* s_id);
void cx_imgui_end_window(struct cx_imgui* p_ctx);

void cx_imgui_separator(struct cx_imgui* p_ctx);
void cx_imgui_text(struct cx_imgui* p_ctx, const char* s_str);
int  cx_imgui_button(struct cx_imgui* p_ctx, const char* s_id, const char* s_str);
int  cx_imgui_text_input(struct cx_imgui* p_ctx, const char* s_id, char* p_buf, size_t buf_len);
int  cx_imgui_checkbox(struct cx_imgui* p_ctx, const char* s_id, int* p_b_value);

int  cx_imgui_begin_dropdown(struct cx_imgui* p_imgui, const char* s_id, const char* s_str);
void cx_imgui_end_dropdown(struct cx_imgui* p_imgui);
int  cx_imgui_dropwdown_item(struct cx_imgui* p_imgui, const char* s_id, const char* s_str);

void  cx_imgui_same_line(struct cx_imgui* p_ctx);
float cx_imgui_get_available_width(struct cx_imgui* p_ctx);
void  cx_imgui_set_next_width(struct cx_imgui* p_ctx, float next_width);

#endif
