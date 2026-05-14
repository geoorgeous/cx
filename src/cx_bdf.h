#ifndef CX_BDF_H
#define CX_BDF_H

#include <stddef.h>

#define CX_LOG_CAT_BDF "bdf"

struct cx_bdf_glyph {
	unsigned int   codepoint_;
	unsigned short width_;
	unsigned short height_;
	short          off_x_;
	short          off_y_;
	short          adv_x_;
	void*          p_bitmap_;
	char           bitmap_bit_offset_;
};

struct cx_bdf {
	void*                p_buf_;
	char*                s_name_;
	size_t               name_len;
	struct cx_bdf_glyph* p_glyphs_;
	size_t               num_glyphs_;
	unsigned short       max_glyph_width_;
	unsigned short       max_glyph_height_;
	unsigned short       line_height_;
};

void cx_bdf_parse(const char* s_bdf_buf, struct cx_bdf* p_out_bdf);

void cx_bdf_free(struct cx_bdf* p_bdf);

#endif
