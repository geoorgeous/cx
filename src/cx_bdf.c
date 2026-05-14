#include <stdlib.h>
#include <string.h>

#include "cx_bdf.h"

#include "cx_logging.h"

#define CX_BDF_KW_STARTFONT        "STARTFONT"
#define CX_BDF_KW_STARTCHAR        "STARTCHAR"
#define CX_BDF_KW_ENDCHAR          "ENDCHAR"
#define CX_BDF_KW_FONT_ASCENT      "FONT_ASCENT"
#define CX_BDF_KW_FONT_DESCENT     "FONT_DESCENT"
#define CX_BDF_KW_FONT_NAME        "FONT"
#define CX_BDF_KW_FONT_BOUNDINGBOX "FONTBOUNDINGBOX"
#define CX_BDF_KW_FONT_CHARCOUNT   "CHARS"
#define CX_BDF_KW_CHAR_ENCODING    "ENCODING"
#define CX_BDF_KW_CHAR_DWIDTH      "DWIDTH"
#define CX_BDF_KW_CHAR_BBX         "BBX"
#define CX_BDF_KW_CHAR_BITMAP      "BITMAP"

static const char* cx_tok_next(const char** pp, size_t* p_out_len);
static int         cx_tok_cmp(const char* p_token, const char* s);
static long        cx_tok_strtol(const char* p_token, size_t token_len);
static void        cx_bdf_allocate_buf(struct cx_bdf* p_bdf, const char* p_font_name, size_t font_name_len);

void cx_bdf_parse(const char* s_bdf_buf, struct cx_bdf* p_out_bdf) {
	CX_LOG(INFO, BDF, "Parsing BDF font...\n");

	*p_out_bdf = (struct cx_bdf){0};

	const char* p_font_name = 0;
	size_t font_name_len = 0;

	char* p_bitmap_pos;
	size_t bitmap_bit_offset;

	struct cx_bdf_glyph* p_glyph = 0;

	const char* p = s_bdf_buf;
	const char* p_tok;
	size_t tok_len;
	while(*p) {
		p_tok = cx_tok_next(&p, &tok_len);

		if (!p_tok) {
			break;
		}
		
		if (cx_tok_cmp(p_tok, CX_BDF_KW_STARTCHAR)) {
			if (!p_out_bdf->p_buf_) {
				cx_bdf_allocate_buf(p_out_bdf, p_font_name, font_name_len);

				p_glyph = p_out_bdf->p_glyphs_;
				p_bitmap_pos = (void*)(p_out_bdf->p_glyphs_ + p_out_bdf->num_glyphs_);
				bitmap_bit_offset = 0;
			}

			*p_glyph = (struct cx_bdf_glyph){
				.p_bitmap_ = p_bitmap_pos,
				.bitmap_bit_offset_ = bitmap_bit_offset
			};
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_ENCODING)) {
			p_glyph->codepoint_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_DWIDTH)) {
			p_glyph->adv_x_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_BBX)) {
			p_glyph->width_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
			p_glyph->height_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
			p_glyph->off_x_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
			p_glyph->off_y_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_BITMAP)) {
			while(1) {
				p_tok = cx_tok_next(&p, &tok_len);
				if (cx_tok_cmp(p_tok, CX_BDF_KW_ENDCHAR)) {
					p_glyph++;
					break;
				}

				const size_t num_bytes = (p_glyph->width_ + 7) / 8;
				for (size_t i_byte = 0; i_byte < num_bytes; ++i_byte) {
					for (size_t i_nibble = 0; i_nibble < 2; ++i_nibble) {
						const char nibble = p_tok[i_byte * 2 + i_nibble];
						/* Convert hex character (0-9, A-F) to value (0-15) */
						const int nibble_value = (nibble & 0xF) + (nibble >> 6) * 9;
						/* Calculate number of bits of the nibble to read, up to four */
						const int bits_remaining = (int)p_glyph->width_ - (i_byte * 8 + i_nibble * 4);
						const int num_bits = bits_remaining < 4 ? bits_remaining : 4;
						for (int i_bit = 0; i_bit < num_bits; ++i_bit) {
							/* Extract bit value from nibble. 0 or 1 */
							const int bit_value = (nibble_value >> (3 - i_bit)) & 1;
							/* Set bit */
							*p_bitmap_pos = (*p_bitmap_pos & ~(1 << bitmap_bit_offset)) | (bit_value << bitmap_bit_offset);
							/* Advance position markers */
							if (++bitmap_bit_offset == 8) {
								bitmap_bit_offset = 0;
								++p_bitmap_pos;
							}
						}
					}
				}
			}
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_STARTFONT)) {
			p_tok = cx_tok_next(&p, &tok_len);
			CX_LOG_FMT(INFO, BDF, "BDF version: %.*s\n", tok_len, p_tok);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_NAME)) {
			p_font_name = cx_tok_next(&p, &font_name_len);
			CX_LOG_FMT(INFO, BDF, "Font name: '%.*s'\n", font_name_len, p_font_name);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_BOUNDINGBOX)) {
			p_out_bdf->max_glyph_width_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
			p_out_bdf->max_glyph_height_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_CHARCOUNT)) {
			p_out_bdf->num_glyphs_ = cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_ASCENT)) {
			p_out_bdf->line_height_ += cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_DESCENT)) {
			p_out_bdf->line_height_ += cx_tok_strtol(cx_tok_next(&p, &tok_len), tok_len);
		}
	}

	const size_t compact_size = (size_t)(p_bitmap_pos - (char*)p_out_bdf->p_buf_);
	p_out_bdf->p_buf_ = realloc(p_out_bdf->p_buf_, compact_size);

	CX_DBG(CX_LOG_FMT(INFO, BDF, "Final buffer size: %d\n", compact_size));
}

void cx_bdf_free(struct cx_bdf* p_bdf) {
	free(p_bdf->p_buf_);
	*p_bdf = (struct cx_bdf){0};
}

const char* cx_tok_next(const char** pp, size_t* p_out_len) {
	while (**pp == ' ' || **pp == '\t' || **pp == '\n') {
		(*pp)++;
	}

	if (!**pp) {
		return 0;
	}

	const char* p_token = *pp;

	while (**pp &&
           **pp != ' ' &&
           **pp != '\t' &&
           **pp != '\n') {
		(*pp)++;
	}

	*p_out_len = (size_t)(*pp - p_token);
	return p_token;
}

int cx_tok_cmp(const char* p_token, const char* s) {
	while(1) {
		if (!*s && (!*p_token || *p_token == ' ' || *p_token == '\t' || *p_token == '\n')) {
			return 1;
		}
		if (*s != *p_token) {
			return 0;
		}
		p_token++;
		s++;
	}
}

long cx_tok_strtol(const char* p_token, size_t token_len) {
	char* p_end;
	long val = strtol(p_token, &p_end, 10);
	if (p_end == p_token || (size_t)(p_end - p_token) != token_len) {
		return 0;
	}
	return val;
};

void cx_bdf_allocate_buf(struct cx_bdf* p_bdf, const char* p_font_name, size_t font_name_len) {
	/* Allocate single buffer containing font name, glyph array, and bitmap buffer */
	const size_t font_name_size = font_name_len + 1;
	const size_t bitmap_atlas_size =
		(p_bdf->max_glyph_width_ * p_bdf->max_glyph_width_ * p_bdf->num_glyphs_ + 7) / 8;
	const size_t glyphs_size = sizeof(*p_bdf->p_glyphs_) * p_bdf->num_glyphs_;
	const size_t buf_size = font_name_size + bitmap_atlas_size + glyphs_size;
	
	p_bdf->p_buf_ = malloc(buf_size);

	p_bdf->s_name_ = p_bdf->p_buf_;
	memcpy(p_bdf->s_name_, p_font_name, font_name_len);
	p_bdf->s_name_[font_name_len] = '\0';

	p_bdf->p_glyphs_ = (void*)(p_bdf->s_name_ + font_name_len + 1);

	CX_DBG(CX_LOG_FMT(INFO, BDF, "Buffer size: %d\n", buf_size));
}

