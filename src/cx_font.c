#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "cx_bits.h"
#include "cx_font.h"
#include "cx_image.h"
#include "cx_logging.h"
#include "cx_pixel_format.h"
#include "cx_texture_atlas_layout.h"
#include "math_utils.h"

struct cx_font_glyph_atlas_dst {
	const struct cx_font_glyph* p_glyph;
	uint32_t x;
	uint32_t y;
};

int cx_font_glyph_atlas_dst_cmp(
	const struct cx_font_glyph_atlas_dst* p_a,
	const struct cx_font_glyph_atlas_dst* p_b);

void cx_font_free_glyph_bitmap_buffer(struct cx_font* p_font) {
	free(p_font->p_glyph_bitmap_buf);
	p_font->p_glyph_bitmap_buf = 0;

	for (size_t i = 0; i < CX_FONT_NUM_GLYPHS; ++i) {
		p_font->glyphs_[i].bitmap_.p_pos = 0;
		p_font->glyphs_[i].bitmap_.bit_offset = 0;
	}
}

int cx_font_find_glyph(const struct cx_font* p_font, uint32_t codepoint, const struct cx_font_glyph** pp_out) {
	if (codepoint >= CX_FONT_NUM_GLYPHS) {
		*pp_out = 0;
		return 0;
	}

	const struct cx_font_glyph* p_glyph = &p_font->glyphs_[codepoint];

	if (!p_glyph->codepoint_) {
		*pp_out = 0;
		return 0;
	}

	*pp_out = p_glyph;
	return 1;
}

void cx_font_create_atlas(
	const struct cx_font* p_font,
	struct cx_image* p_out_atlas,
	struct cx_texture_atlas_layout* p_out_layout) {
	
	// Sort glyph atlas rects
	struct cx_font_glyph_atlas_dst glyph_atlas_dsts[CX_FONT_NUM_GLYPHS];
	for (size_t i = 0; i < CX_FONT_NUM_GLYPHS; ++i) {
		glyph_atlas_dsts[i] = (struct cx_font_glyph_atlas_dst) {
			.p_glyph = p_font->glyphs_ + i,
		};
	}
	qsort(
		glyph_atlas_dsts,
		CX_FONT_NUM_GLYPHS,
		sizeof(glyph_atlas_dsts[0]),
		(int(*)(const void*, const void*))cx_font_glyph_atlas_dst_cmp);
	
	// Pack glyph atlas rects together
	const uint32_t area = p_font->max_glyph_width_ * p_font->max_glyph_height_ * CX_FONT_NUM_GLYPHS;
	const uint32_t area_sqrt = (uint32_t)sqrtf((float)area);
	const uint32_t width = next_pow2_uint32(area_sqrt) / 2;
	
	uint32_t row_right = 0;
	uint32_t row_top = 0;
	uint32_t row_height = 0;
	for (size_t i = 0; i < CX_FONT_NUM_GLYPHS; ++i) {
		if (!glyph_atlas_dsts[i].p_glyph->codepoint_ ||
			glyph_atlas_dsts[i].p_glyph->metrics_.width == 0 ||
			glyph_atlas_dsts[i].p_glyph->metrics_.height == 0) {
			continue;
		}

		if (row_right + glyph_atlas_dsts[i].p_glyph->metrics_.width > width) {
			row_right = 0;
			row_top += row_height;
			row_height = 0;
		}

		if (glyph_atlas_dsts[i].p_glyph->metrics_.height > row_height) {
			row_height = glyph_atlas_dsts[i].p_glyph->metrics_.height;
		}

		glyph_atlas_dsts[i].x = row_right;
		glyph_atlas_dsts[i].y = row_top;

		row_right += glyph_atlas_dsts[i].p_glyph->metrics_.width;
	}

	const uint32_t height = next_pow2_uint32(row_top + row_height);

	uint8_t* p_pixels = calloc(1, width * height);

	// Compute UVs and copy glyph bitmaps in to image
	for (size_t i = 0; i < CX_FONT_NUM_GLYPHS; ++i) {
		const struct cx_font_glyph_atlas_dst* p_dst = &glyph_atlas_dsts[i];

		const size_t glyph_index = (size_t)(glyph_atlas_dsts[i].p_glyph - p_font->glyphs_);
		
		if (!glyph_atlas_dsts[i].p_glyph->codepoint_ ||
			glyph_atlas_dsts[i].p_glyph->metrics_.width == 0 ||
			glyph_atlas_dsts[i].p_glyph->metrics_.height == 0) {
			p_out_layout->p_entries[glyph_index] = (struct cx_texture_atlas_entry){0};
			continue;
		}

		const struct cx_font_glyph* p_glyph = p_dst->p_glyph;
		
		p_out_layout->p_entries[glyph_index] = (struct cx_texture_atlas_entry) {
			.u0 = (float)p_dst->x / (float)width,
			.u1 = (float)(p_dst->x + p_glyph->metrics_.width) / (float)width,
			.v0 = 1.0f - (float)(p_dst->y + p_glyph->metrics_.height) / (float)height,
			.v1 = 1.0f - (float)(p_dst->y) / (float)height
		};

		for (size_t y = 0; y < p_glyph->metrics_.height; ++y) {
			uint8_t* p = p_pixels + width * (height - 1 - (p_dst->y + y)) + p_dst->x;
			
			for (size_t x = 0; x < p_glyph->metrics_.width; ++x) {
				const size_t bit = y * p_glyph->metrics_.width + x;
				
				const size_t src_bit_n = p_glyph->bitmap_.bit_offset + bit;
				
				const uint8_t src_byte = ((uint8_t*)p_glyph->bitmap_.p_pos)[src_bit_n / 8];
				
				*p = 0xFF * CX_BIT_GET(src_byte, src_bit_n % 8);
				
				p++;
			}
		}
	}

	CX_LOG_FMT(INFO, FONT, "Font atlas built: Size=%ux%u\n", width, height);

	*p_out_atlas = (struct cx_image) {
		.width = width,
		.height = height,
		.pixel_data_format = {
			.pixel_format = CX_PIXEL_FORMAT_red,
			.pixel_type = CX_PIXEL_TYPE_u8
		},
		.p_pixel_data = p_pixels
	};
}

int cx_font_glyph_atlas_dst_cmp(
	const struct cx_font_glyph_atlas_dst* p_a,
	const struct cx_font_glyph_atlas_dst* p_b) {
	
	return
		p_b->p_glyph->metrics_.width * p_b->p_glyph->metrics_.height -
		p_a->p_glyph->metrics_.width * p_a->p_glyph->metrics_.height;
}
