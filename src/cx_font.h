#ifndef CX_FONT_H
#define CX_FONT_H

#include <stddef.h>
#include <stdint.h>

#include "cx_macro.h"

#define CX_ASSET_TYPE_FONT 6

#define CX_LOG_CAT_FONT "font"

#define CX_FONT_NUM_GLYPHS 256

struct cx_font_glyph {
	unsigned int   codepoint_;
	struct {
		unsigned short width;
		unsigned short height;
		short          off_x;
		short          off_y;
		short          adv_x;
	} metrics_;
	CX_PADDING(2);
	struct {
		void*   p_pos;
		uint8_t bit_offset;
		CX_PADDING(7);
	} bitmap_;
};

struct cx_font {
	struct cx_font_glyph glyphs_[CX_FONT_NUM_GLYPHS];
	unsigned short       max_glyph_width_;
	unsigned short       max_glyph_height_;
	unsigned short       line_height_;
	unsigned short       descent_;
	unsigned short       space_adv_;
	void*                p_glyph_bitmap_buf;
	};

void cx_font_free_glyph_bitmap_buffer(struct cx_font* p_font);

int cx_font_find_glyph(const struct cx_font* p_font, uint32_t codepoint, const struct cx_font_glyph** pp_out);

struct cx_image;
struct cx_texture_atlas_layout;

void cx_font_create_atlas(
	const struct cx_font* p_font,
	struct cx_image* p_out_atlas,
	struct cx_texture_atlas_layout* p_out_layout);

#endif
