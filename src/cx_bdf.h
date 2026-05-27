#ifndef CX_BDF_H
#define CX_BDF_H

#include <stddef.h>
#include <stdint.h>

#define CX_LOG_CAT_BDF "bdf"

struct cx_bdf_glyph {
	uint32_t codepoint_;
	uint16_t width_;
	uint16_t height_;
	int16_t  off_x_;
	int16_t  off_y_;
	int16_t  adv_x_;
	void*    p_bitmap_;
	uint8_t  bitmap_bit_offset_;
};

struct cx_bdf {
	void*  p_buf_;
	char*  s_name_;
	size_t name_len;
	struct cx_bdf_glyph* p_glyphs_;
	size_t   num_glyphs_;
	uint16_t max_glyph_width_;
	uint16_t max_glyph_height_;
	uint16_t line_height_;
	uint16_t descent_;
};

void cx_bdf_parse(const char* s_bdf_buf, struct cx_bdf* p_out_bdf);

void cx_bdf_free(struct cx_bdf* p_bdf);

#endif
