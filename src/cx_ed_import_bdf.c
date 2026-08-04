#include "cx_alloc.h"
#include "cx_bdf.h"
#include "cx_bits.h"
#include "cx_ed_asset_library.h"
#include "cx_ed_import_bdf.h"
#include "cx_font.h"
#include "cx_io.h"
#include "cx_logging.h"

int cx_ed_import_bdf(const char* s_name, const struct cx_bdf* p_bdf, struct cx_asset_ref* p_out) {
	struct cx_font* p_font = CX_MALLOC(sizeof(struct cx_font));
	*p_font = (struct cx_font) {
		.max_glyph_width_ = p_bdf->max_glyph_width_,
		.max_glyph_height_ = p_bdf->max_glyph_height_,
		.line_height_ = p_bdf->line_height_,
		.descent_ = p_bdf->descent_
	};

	const size_t buf_size = (p_bdf->max_glyph_width_ * p_bdf->max_glyph_height_ * CX_FONT_NUM_GLYPHS - 7u) / 8u;
	p_font->p_glyph_bitmap_buf = CX_MALLOC(buf_size);

	size_t bitmap_offset = 0;
	uint8_t bitmap_bit_offset = 0;

	int num_glyphs_read = 0;

	for (size_t i = 0; i < p_bdf->num_glyphs_; ++i) {
		const struct cx_bdf_glyph* p_bdf_glyph = p_bdf->p_glyphs_ + i;	

		if (p_bdf_glyph->codepoint_ >= CX_FONT_NUM_GLYPHS) {
			continue;
		}

		++num_glyphs_read;

		struct cx_font_glyph* p_glyph = &p_font->glyphs_[p_bdf_glyph->codepoint_];
		*p_glyph = (struct cx_font_glyph) {
			.codepoint_ = p_bdf_glyph->codepoint_,
			.metrics_ = {
				.width = p_bdf_glyph->width_,
				.height = p_bdf_glyph->height_,
				.off_x = p_bdf_glyph->off_x_,
				.off_y = p_bdf_glyph->off_y_,
				.adv_x = p_bdf_glyph->adv_x_
			},
			.bitmap_ = {
				.offset = bitmap_offset,
				.bit_offset = bitmap_bit_offset
			}
		};
		
		const size_t num_bits = p_glyph->metrics_.width * p_glyph->metrics_.height;

		cx_bits_copy(
			(uint8_t*)p_font->p_glyph_bitmap_buf + p_glyph->bitmap_.offset,
			p_glyph->bitmap_.bit_offset,
			p_bdf_glyph->p_bitmap_,
			p_bdf_glyph->bitmap_bit_offset_,
			num_bits);

		bitmap_offset += (bitmap_bit_offset + num_bits) / 8;
		bitmap_bit_offset = (bitmap_bit_offset + num_bits) % 8;;
	}

	CX_LOG_FMT(INFO, IMPORT_BDF, "Font imported from BDF font: %d glyphs read, glyph bitmap buffer size=%llu\n",
		num_glyphs_read,
		buf_size);

	const struct cx_font_glyph* p_space_glyph;
	if (cx_font_find_glyph(p_font, (uint32_t)' ', &p_space_glyph)) {
		p_font->space_adv_ = p_space_glyph->metrics_.adv_x;
	} else {
		p_font->space_adv_ = (int32_t)p_font->max_glyph_width_;
	}

	cx_ed_asset_library_new(CX_ASSET_TYPE_FONT, s_name, p_font, p_out);

	return CX_TRUE;
}

int cx_ed_import_bdf_file(const char* s_filepath, struct cx_asset_ref* p_out) {

	void* p_bdf_buf;
	size_t bdf_buf_size;
	if (cx_io_file_read_all(s_filepath, &p_bdf_buf, &bdf_buf_size) != CX_ERROR_none) {
		return CX_FALSE;
	}

	struct cx_bdf bdf;
	cx_bdf_parse(p_bdf_buf, &bdf);
	cx_io_file_free(p_bdf_buf);

	char asset_name_buf[CX_ASSET_NAME_MAX_LEN];
	size_t asset_name_len;
	cx_io_filepath_stem_cpy(s_filepath, asset_name_buf, &asset_name_len);

	const int result = cx_ed_import_bdf(asset_name_buf, &bdf, p_out);

	cx_bdf_free(&bdf);

	return result;
}
