#ifndef CX_FONT_H
#define CX_FONT_H

#include <stddef.h>
#include <stdint.h>

#define CX_ASSET_TYPE_FONT 6

#define CX_LOG_CAT_FONT "font"

#define CX_FONT_NUM_GLYPHS 256u

struct cx_font_glyph {
	uint32_t codepoint_;
	struct {
		uint32_t width;
		uint32_t height;
		int32_t  off_x;
		int32_t  off_y;
		int32_t  adv_x;
	} metrics_;
	struct {
		void*   p_pos;
		uint8_t bit_offset;
	} bitmap_;
};

struct cx_font {
	struct cx_font_glyph glyphs_[CX_FONT_NUM_GLYPHS];
	uint32_t max_glyph_width_;
	uint32_t max_glyph_height_;
	uint32_t line_height_;
	int32_t  descent_;
	int32_t  space_adv_;
	void*    p_glyph_bitmap_buf;
};

void cx_font_free_glyph_bitmap_buffer(struct cx_font* p_font);

int cx_font_find_glyph(const struct cx_font* p_font, uint32_t codepoint, const struct cx_font_glyph** pp_out);

struct cx_image;
struct cx_texture_atlas_layout;

void cx_font_create_atlas(
	const struct cx_font* p_font,
	struct cx_image* p_out_atlas,
	struct cx_texture_atlas_layout* p_out_layout);

void cx_asset_free_font(void* p);

#endif
