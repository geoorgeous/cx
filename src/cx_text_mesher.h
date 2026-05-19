#ifndef CX_TEXT_MESHER_H
#define CX_TEXT_MESHER_H

#include <stddef.h>
#include <stdint.h>

#include "cx_color.h"

struct cx_font;
struct cx_gfx_texture;
struct cx_texture_atlas_layout;

struct cx_font_render_data {
	const struct cx_font*                 p_font;
	const struct cx_texture_atlas_layout* p_glyph_atlas_layout;
	const struct cx_gfx_texture*          p_glyph_texture;
};

struct cx_text_mesh_desc {
	const char*         s_text;
	struct cx_color_f32 color;
	float               position[3];
	float               scale;
	struct cx_font_render_data font_data;
};

void cx_text_mesher_measure(const struct cx_text_mesh_desc* p_desc);

struct mesh_primitive;

void cx_text_mesher_generate(
	const struct cx_text_mesh_desc* p_descs,
	size_t num_descs,
	struct mesh_primitive* p_out_mesh);

void cx_text_mesher_free(struct mesh_primitive* p_mesh);

#endif
