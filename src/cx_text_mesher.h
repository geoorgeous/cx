#ifndef CX_TEXT_MESHER_H
#define CX_TEXT_MESHER_H

#include <stddef.h>
#include <stdint.h>

#include "cx_color.h"
#include "cx_mesh_data.h"

struct cx_font;
struct cx_gfx_texture;
struct cx_texture_atlas_layout;

struct cx_font_render_data {
	const struct cx_font*                 p_font;
	const struct cx_texture_atlas_layout* p_glyph_atlas_layout;
	const struct cx_gfx_texture*          p_glyph_texture;
};

struct cx_text_style {
	const struct cx_font_render_data* p_font_render_data;
	float scale;
	struct cx_color color;
};

struct cx_text_span {
	size_t offset;
	size_t len;
	struct cx_text_style style;
};

struct cx_text_mesher_input {
	const char*          s_text;
	float                position[3];
	struct cx_text_style style;
	const struct cx_text_span* p_spans;
	size_t num_spans;
};

struct cx_text_mesher_output {
	struct cx_mesh_data mesh_data;
	const struct cx_gfx_texture* p_atlas_texture;
};

void cx_text_mesher_measure(
	const char* s,
	size_t n,
	const struct cx_font_render_data* p_font_render_data,
	float scale,
	float* p_out_x, float* p_out_y);

void cx_text_mesher_generate(
	const struct cx_text_mesher_input* p_input,
	size_t max_meshes,
	struct cx_text_mesher_output* p_outputs,
	size_t* p_out_num_outputs);

void cx_text_mesher_free(struct cx_text_mesher_output* p_output, size_t n);

#endif
