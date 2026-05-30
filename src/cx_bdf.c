#include <string.h>

#include "cx_alloc.h"
#include "cx_bdf.h"
#include "cx_dbg.h"
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

int cx_bdf_parse(const char* s_bdf_buf, struct cx_bdf* p_out_bdf) {
	CX_LOG(INFO, BDF, "Parsing BDF font...\n");

	*p_out_bdf = (struct cx_bdf){0};

	const char* p_font_name = 0;
	size_t font_name_len = 0;

	char* p_bitmap_pos = 0;
	uint8_t bitmap_bit_offset = 0;

	struct cx_bdf_glyph* p_glyph = 0;

	const char* s_errmsg = 0;
	int b_parsing_char = CX_FALSE;

	const char* p = s_bdf_buf;
	const char* p_tok;
	size_t tok_len;
	while(*p) {
		p_tok = cx_tok_next(&p, &tok_len);

		if (!p_tok) {
			break;
		}
		
		if (cx_tok_cmp(p_tok, CX_BDF_KW_STARTCHAR)) {
			if (b_parsing_char) {
				s_errmsg = "STARTCHAR encountered before end of previous character";
goto cx_bdf_parse_error;
			}

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

			b_parsing_char = CX_TRUE;
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_ENCODING)) {
			if (!b_parsing_char) {
				s_errmsg = "CHAR_ENCODING encountered before start of character";
goto cx_bdf_parse_error;
			}

			p_tok = cx_tok_next(&p, &tok_len);
			p_glyph->codepoint_ = (uint32_t)cx_tok_strtol(p_tok, tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_DWIDTH)) {
			if (!b_parsing_char) {
				s_errmsg = "CHAR_WIDTH encountered before start of character";
goto cx_bdf_parse_error;
			}

			p_tok = cx_tok_next(&p, &tok_len);
			p_glyph->adv_x_ = (int16_t)cx_tok_strtol(p_tok, tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_BBX)) {
			if (!b_parsing_char) {
				s_errmsg = "CHAR_BBX encountered before start of character";
goto cx_bdf_parse_error;
			}

			p_tok = cx_tok_next(&p, &tok_len);
			p_glyph->width_  = (uint16_t)cx_tok_strtol(p_tok, tok_len);
			p_tok = cx_tok_next(&p, &tok_len);
			p_glyph->height_ = (uint16_t)cx_tok_strtol(p_tok, tok_len);
			p_tok = cx_tok_next(&p, &tok_len);
			p_glyph->off_x_  =  (int16_t)cx_tok_strtol(p_tok, tok_len);
			p_tok = cx_tok_next(&p, &tok_len);
			p_glyph->off_y_  =  (int16_t)cx_tok_strtol(p_tok, tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_CHAR_BITMAP)) {
			if (!b_parsing_char) {
				s_errmsg = "CHAR_BITMAP encountered before start of character";
goto cx_bdf_parse_error;
			}

			for (size_t i = 0; i <= p_glyph->height_; ++i) {
				p_tok = cx_tok_next(&p, &tok_len);
				if (cx_tok_cmp(p_tok, CX_BDF_KW_ENDCHAR)) {
					p_glyph++;
					b_parsing_char = CX_FALSE;
					break;
				}

				const size_t num_bytes = (p_glyph->width_ + 7u) / 8u;

				for (size_t i_byte = 0; i_byte < num_bytes; ++i_byte) {
					for (size_t i_nibble = 0; i_nibble < 2; ++i_nibble) {
						const uint8_t nibble = (uint8_t)p_tok[i_byte * 2u + i_nibble];
						/* Convert hex character (0-9, A-F) to value (0-15) */
						const uint8_t nibble_value = (uint8_t)((nibble & 0xFu) + (nibble >> 6u) * 9u);
						/* Calculate number of bits of the nibble to read, up to four */
						const size_t bits_remaining = p_glyph->width_ - (i_byte * 8 + i_nibble * 4u);
						const uint8_t num_bits = bits_remaining < 4u ? (uint8_t)bits_remaining : 4u;
						for (size_t i_bit = 0; i_bit < num_bits; ++i_bit) {
							/* Extract bit value from nibble. 0 or 1 */
							const int bit_value = (nibble_value >> (3u - i_bit)) & 1u;
							/* Set bit */
							*p_bitmap_pos = (char)((*p_bitmap_pos & ~(1 << bitmap_bit_offset))
								| (bit_value << bitmap_bit_offset));
							/* Advance position markers */
							if (++bitmap_bit_offset == 8) {
								bitmap_bit_offset = 0;
								++p_bitmap_pos;
							}
						}
					}
				}
			}

			if (b_parsing_char) {
				s_errmsg = "Expected ENDCHAR after bitmap";
goto cx_bdf_parse_error;
			}
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_STARTFONT)) {
			p_tok = cx_tok_next(&p, &tok_len);
			CX_LOG_FMT(INFO, BDF, "BDF version: %.*s\n", tok_len, p_tok);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_NAME)) {
			p_font_name = cx_tok_next(&p, &font_name_len);
			CX_LOG_FMT(INFO, BDF, "Font name: '%.*s'\n", font_name_len, p_font_name);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_BOUNDINGBOX)) {
			p_tok = cx_tok_next(&p, &tok_len);
			p_out_bdf->max_glyph_width_ = (uint16_t)cx_tok_strtol(p_tok, tok_len);
			p_tok = cx_tok_next(&p, &tok_len);
			p_out_bdf->max_glyph_height_ = (uint16_t)cx_tok_strtol(p_tok, tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_CHARCOUNT)) {
			p_tok = cx_tok_next(&p, &tok_len);
			p_out_bdf->num_glyphs_ = (uint16_t)cx_tok_strtol(p_tok, tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_ASCENT)) {
			p_tok = cx_tok_next(&p, &tok_len);
			p_out_bdf->line_height_ += (uint16_t)cx_tok_strtol(p_tok, tok_len);
		} else if (cx_tok_cmp(p_tok, CX_BDF_KW_FONT_DESCENT)) {
			p_tok = cx_tok_next(&p, &tok_len);
			p_out_bdf->descent_ = (uint16_t)cx_tok_strtol(p_tok, tok_len);
			p_out_bdf->line_height_ += p_out_bdf->descent_;
		}
	}

	return CX_TRUE;

cx_bdf_parse_error:

	CX_LOG_FMT(ERROR, BDF, "Failed to parse BDF buffer: %s\n", s_errmsg);

	cx_bdf_free(p_out_bdf);

	return CX_FALSE;
}

void cx_bdf_free(struct cx_bdf* p_bdf) {
	CX_FREE(p_bdf->p_buf_);
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
			return CX_TRUE;
		}
		if (*s != *p_token) {
			return CX_FALSE;
		}
		p_token++;
		s++;
	}
}

int64_t cx_tok_strtol(const char* p_token, size_t token_len) {
	char* p_end;
	int64_t val = (int64_t)strtol(p_token, &p_end, 10);
	if ((p_end == p_token) || ((size_t)(p_end - p_token) != token_len)) {
		return 0;
	}
	return val;
}

void cx_bdf_allocate_buf(struct cx_bdf* p_bdf, const char* p_font_name, size_t font_name_len) {
	/* Allocate single buffer containing font name, glyph array, and bitmap buffer */
	const size_t glyphs_size = sizeof(*p_bdf->p_glyphs_) * p_bdf->num_glyphs_;
	const size_t bitmap_atlas_size =
		(p_bdf->max_glyph_width_ * p_bdf->max_glyph_width_ * p_bdf->num_glyphs_ + 7) / 8;
	const size_t font_name_size = font_name_len + 1;
	
	const size_t buf_size = glyphs_size + bitmap_atlas_size + font_name_size;
	
	p_bdf->p_buf_ = CX_MALLOC(buf_size);
	p_bdf->p_glyphs_ = p_bdf->p_buf_;
	p_bdf->s_name_ = (char*)p_bdf->p_buf_ + glyphs_size + bitmap_atlas_size;

	memcpy(p_bdf->s_name_, p_font_name, font_name_len);
	p_bdf->s_name_[font_name_len] = '\0';

	CX_DBG(CX_LOG_FMT(INFO, BDF, "Buffer size: %d\n", buf_size));
}

