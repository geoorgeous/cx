#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"
#include "otf.h"
#include "serialization.h"

#define OTF_LOG_CAT "OTF"

#define OTF_TABLE_DIRECTORY_SIZE 12
#define OTF_TABLE_RECORD_SIZE 16
#define OTF_TABLE_RECORD_TAG_SIZE 4

#define OTF_TABLE_TAG_HEAD "head"
#define OTF_TABLE_TAG_NAME "name"
#define OTF_TABLE_TAG_HHEA "hhea"
#define OTF_TABLE_TAG_MAXP "maxp"
#define OTF_TABLE_TAG_HMTX "hmtx"
#define OTF_TABLE_TAG_CMAP "cmap"
#define OTF_TABLE_TAG_CFF  "CFF "

struct otf_cff_index_st {
	uint16_t  count;
	uint32_t* offset_arr;
	uint8_t*  data;
};

struct otf_cff_top_dict_st {
	uint32_t char_strings_index_offset;
	uint32_t char_string_type;
	uint32_t private_dict_offset;
	uint32_t private_dict_size;
};

struct otf_cff_private_dict_st {
	uint32_t local_subr_offset;
	int32_t default_width_x;
	int32_t nominal_width_x;
};

static const struct otf_table_rec_st* otf_find_table_rec(const struct otf_table_rec_st* table_rec_arr_p, uint16_t num_tables, const char* tag_str) {
	for (uint16_t i = 0; i < num_tables; ++i) {
		char table_tag_chars[sizeof(table_rec_arr_p[i].tag)] = {0};
		for (size_t j = 0; j < sizeof(table_rec_arr_p[i].tag) - 1; ++j)
			table_tag_chars[j] = (char)table_rec_arr_p[i].tag[j];
		if (strcmp(table_tag_chars, tag_str) == 0)
			return table_rec_arr_p + i;
	}
	return NULL;
}

static void* otf_load_table(const struct otf_table_rec_st* table_rec_p, FILE* otf_file_p) {
	fseek(otf_file_p, table_rec_p->offset, SEEK_SET);
	void* table_buf_p = malloc(table_rec_p->length);
	fread(table_buf_p, table_rec_p->length, 1, otf_file_p);
	return table_buf_p;
}

static void otf_free_table(void* table_buf_p) {
	free(table_buf_p);
}

static void otf_cff_load_index(void** buf_pp, struct otf_cff_index_st* index_p) {
	*index_p = (struct otf_cff_index_st){0};

	serialization_read_uint16(buf_pp, &index_p->count);
	
	log_msg(LOG_DEBUG, OTF_LOG_CAT, "INDEX.count: %hu\n", index_p->count);

	if (index_p->count == 0)
		return;

	uint8_t offset_elem_size;
	serialization_read_uint8(buf_pp, &offset_elem_size);
	
	log_msg(LOG_DEBUG, OTF_LOG_CAT, "INDEX.offSize: %hhu\n", offset_elem_size);

	index_p->offset_arr = malloc(sizeof(*index_p->offset_arr) * (index_p->count + 1));
	
	for (uint16_t i = 0; i < index_p->count + 1; ++i) {
		uint32_t* offset_elem_p = index_p->offset_arr + i;
		switch (offset_elem_size) {
			case 1: {
				uint8_t t;
				serialization_read_uint8(buf_pp, &t);
				*offset_elem_p = t;
				break;
			}

			case 2: {
				uint16_t t;
				serialization_read_uint16(buf_pp, &t);
				*offset_elem_p = t;
				break;
			}

			case 3: {
				uint8_t t[3];
				serialization_read_bytes(buf_pp, t, 3);
				*offset_elem_p = (uint32_t)t[0] << 16 | (uint32_t)t[1] << 8 | (uint32_t)t[2];
				break;
			}

			case 4: {
				serialization_read_uint32(buf_pp, offset_elem_p);
				break;
			}
		}
	}

	index_p->data = *buf_pp;
	*buf_pp = (unsigned char*)index_p->data + index_p->offset_arr[index_p->count] - 1;
}

static void otf_cff_free_index(struct otf_cff_index_st* index_p) {
	free(index_p->offset_arr);
}

struct cff_dict_number_st {
	int b_is_real;
	union {
		int32_t  as_int32;
		float    as_float;
	} value;
};

static void otf_cff_dict_decode_number(void** dict_operand_pp, struct cff_dict_number_st* number) {
	*number = (struct cff_dict_number_st){0};
	uint8_t* b0_p = *dict_operand_pp;
	uint8_t  b0 = *b0_p;
	if (b0 >= 32 && b0 <= 246) {
		// Operand Value [-107 ... 107] = b0 - 139;
		number->value.as_int32 = b0 - 139;
		*dict_operand_pp += 1;
	} else if (b0 >= 247 && b0 <= 250) {
		// Operand Value [108 ... 1131] = (b0 - 247) * 256 + b1 + 108
		number->value.as_int32 = (b0 - 247) * 256 + b0_p[1] + 108;
		*dict_operand_pp += 2;
	} else if (b0 >= 251 && b0 <= 254) {
		// Operand Value [-1131 ... -108] = -(b0 - 251) * 256 - b1 -108]
		number->value.as_int32 = -(b0 - 251) * 256 - b0_p[1] - 108;
		*dict_operand_pp += 2;
	} else if (b0 == 28) {
		// Operand Value [-32768 ... 32767] = b1 << 8 | b2
		number->value.as_int32 = (uint32_t)b0_p[1] << 8 | b0_p[2];
		*dict_operand_pp += 3;
	} else if (b0 == 29) {
		// Operand Value [-2^31 ... 2^31 - 1] = b1 << 24 | b2 << 16 | b3 << 8 | b4
		number->value.as_int32 = (uint32_t)b0_p[1] << 24 | (uint32_t)b0_p[2] << 16 | (uint32_t)b0_p[3] << 8 | b0_p[4];
		*dict_operand_pp += 5;
	} else if (b0 == 30) {
		// Operand Value real number
		number->b_is_real = 1;

		char str[32];
		char* cp = str;

		int b_lower_bits = 0;
		uint8_t* byte_p = b0_p + 1;
		while (1) {
			uint8_t nibble = (b_lower_bits) ? (*byte_p & 0x0F) : (*byte_p >> 4);
			if (b_lower_bits) byte_p++;
			b_lower_bits = !b_lower_bits;

			if (nibble <= 0x9) {
				*cp++ = '0' + nibble;
			} else if (nibble == 0xE) {
				*cp++ = '-';
			} else if (nibble == 0xA) {
				*cp++ = '.';
			} else if (nibble == 0xB) {
				*cp++ = 'e';
			} else if (nibble == 0xC) {
				*cp++ = 'e';
				*cp++ = '-';
			} else if (nibble == 0xF) {
				*cp = '\0';
				number->value.as_float = atof(str);
				break;
			}
		}
		*dict_operand_pp = byte_p + 1;
	} else {
		*dict_operand_pp += 1;
	}
}

static void otf_cff_decode_dict(void* dict_p, size_t dict_size, void(*operator_handler)(const struct cff_dict_number_st*, size_t, const uint8_t*, void*), void* user_p) {
	struct cff_dict_number_st decoded_operands[48];
	size_t num_decoded_operands = 0;

	void* dict_pos_p = dict_p;
	while ((size_t)(dict_pos_p - dict_p) < dict_size) {
		const uint8_t* b0_p = dict_pos_p;
		const uint8_t  b0 = *b0_p;

		if (b0 <= 21) {

			dict_pos_p += (b0 == 12) ? 2 : 1;

			operator_handler(decoded_operands, num_decoded_operands, b0_p, user_p);

			num_decoded_operands = 0;
			
			// if (b0 == 12)
			// 	log_msg(LOG_DEBUG, OTF_LOG_CAT, "DICT operator: %hhu %hhu\n", b0_p[0], b0_p[1]);
			// else
			// 	log_msg(LOG_DEBUG, OTF_LOG_CAT, "DICT operator: %hhu\n", b0_p[0]);
		} else {
			struct cff_dict_number_st* operand_p = &decoded_operands[num_decoded_operands];
			otf_cff_dict_decode_number(&dict_pos_p, operand_p);
			num_decoded_operands++;

			// if (operand_p->b_is_real)
			// 	log_msg(LOG_DEBUG, OTF_LOG_CAT, "DICT operand: %f (float)\n", operand_p->value.as_float);	
			// else
			// 	log_msg(LOG_DEBUG, OTF_LOG_CAT, "DICT operand: %d (int32)\n", operand_p->value.as_int32);	
		}
	}
}

static void otf_cff_handle_top_dict_operator(const struct cff_dict_number_st* decoded_operand_arr, size_t decoded_operand_arr_n, const uint8_t* operator_p, void* user_p) {
	struct otf_cff_top_dict_st* top_dict_p = user_p;

	if (operator_p[0] == 17) {
		top_dict_p->char_strings_index_offset = decoded_operand_arr[decoded_operand_arr_n - 1].value.as_int32;
	} else if (operator_p[0] == 18) {
		top_dict_p->private_dict_offset = decoded_operand_arr[decoded_operand_arr_n - 1].value.as_int32;
		top_dict_p->private_dict_size = decoded_operand_arr[decoded_operand_arr_n - 2].value.as_int32;
	} else if (operator_p[0] == 12 && operator_p[1] == 6) {
		top_dict_p->char_string_type = decoded_operand_arr[decoded_operand_arr_n - 1].value.as_int32;
	}
}

static void otf_cff_handle_private_dict_operator(const struct cff_dict_number_st* decoded_operand_arr, size_t decoded_operand_arr_n, const uint8_t* operator_p, void* user_p) {
	struct otf_cff_private_dict_st* private_dict_p = user_p;

	if (operator_p[0] == 19) {
		private_dict_p->local_subr_offset = decoded_operand_arr[decoded_operand_arr_n - 1].value.as_int32;
	} else if (operator_p[0] == 20) {
		private_dict_p->default_width_x = decoded_operand_arr[decoded_operand_arr_n - 1].value.as_int32;
	} else if (operator_p[0] == 21) {
		private_dict_p->nominal_width_x = decoded_operand_arr[decoded_operand_arr_n - 1].value.as_int32;
	}
}

void otf_load(const char* filepath_str, struct otf_st* otf_p) {
	FILE* file_p = fopen(filepath_str, "rb");
	if (!file_p) {
		log_msg(LOG_DEBUG, OTF_LOG_CAT, "Failed to open file '%s'\n", filepath_str);
		return;
	}

	log_msg(LOG_DEBUG, OTF_LOG_CAT, "Processing file '%s'\n", filepath_str);

	serialization_set_byte_order(SERIALIZATION_BYTE_ORDER_big_endian);

	void* buf_p;

	char table_director_buf[OTF_TABLE_DIRECTORY_SIZE];
	fread(table_director_buf, sizeof(table_director_buf), 1, file_p);
	buf_p = table_director_buf;

	uint32_t nsft_version;
	serialization_read_uint32(&buf_p, &nsft_version);

	uint16_t num_tables;
	serialization_read_uint16(&buf_p, &num_tables);

	struct otf_table_rec_st* table_record_arr = malloc(sizeof(struct otf_table_rec_st) * num_tables);

	for (uint16_t i = 0; i < num_tables; ++i) {
		struct otf_table_rec_st* record_p = table_record_arr + i;

		char table_record_buf[OTF_TABLE_RECORD_SIZE];
		fread(table_record_buf, sizeof(table_record_buf), 1, file_p);
		buf_p = table_record_buf;

		serialization_read_bytes(&buf_p, record_p->tag, OTF_TABLE_RECORD_TAG_SIZE);
		record_p->tag[OTF_TABLE_RECORD_TAG_SIZE] = 0;

		serialization_read_uint32(&buf_p, &record_p->checksum);
		serialization_read_uint32(&buf_p, &record_p->offset);
		serialization_read_uint32(&buf_p, &record_p->length);

		// log_msg(LOG_DEBUG, OTF_LOG_CAT, "Table record [%hu]:\n\ttag: '%s'\n\tchecksum: %u\n\toffset: %u\n\tlength: %u\n", i, record_p->tag, record_p->checksum, record_p->offset, record_p->length);
	}

	const struct otf_table_rec_st* record_p;
	void* table_p;

	// HEAD TABLE
	{
		record_p = otf_find_table_rec(table_record_arr, num_tables, OTF_TABLE_TAG_HEAD);
		table_p = otf_load_table(record_p, file_p);

		buf_p = table_p;

		serialization_read_uint16(&buf_p, &otf_p->head.version_major);
		serialization_read_uint16(&buf_p, &otf_p->head.version_minor);
		serialization_read_uint32(&buf_p, &otf_p->head.revision_number_fp);
		serialization_read_uint32(&buf_p, &otf_p->head.checksum_adjustment);
		serialization_read_uint32(&buf_p, &otf_p->head.magix_number);
		serialization_read_uint16(&buf_p, &otf_p->head.flags);
		serialization_read_uint16(&buf_p, &otf_p->head.units_per_em);
		serialization_read_int64(&buf_p, &otf_p->head.datetime_created);
		serialization_read_int64(&buf_p, &otf_p->head.datetime_modified);
		serialization_read_int16(&buf_p, &otf_p->head.x_min);
		serialization_read_int16(&buf_p, &otf_p->head.y_min);
		serialization_read_int16(&buf_p, &otf_p->head.x_max);
		serialization_read_int16(&buf_p, &otf_p->head.y_max);
		serialization_read_uint16(&buf_p, &otf_p->head.mac_style);
		serialization_read_uint16(&buf_p, &otf_p->head.lowest_rec_ppem);
		serialization_read_int16(&buf_p, &otf_p->head.font_direction_hint);
		serialization_read_int16(&buf_p, &otf_p->head.index_to_loc_format);
		serialization_read_int16(&buf_p, &otf_p->head.glyph_data_format);

		otf_free_table(table_p);
	}

	// NAME TABLE
	{
		record_p = otf_find_table_rec(table_record_arr, num_tables, OTF_TABLE_TAG_NAME);
		table_p = otf_load_table(record_p, file_p);

		buf_p = table_p;

		serialization_read_uint16(&buf_p, &otf_p->name.version);
		serialization_read_uint16(&buf_p, &otf_p->name.count);
		serialization_read_uint16(&buf_p, &otf_p->name.storage_offset);

		otf_p->name.name_record_arr = malloc(sizeof(*otf_p->name.name_record_arr) * otf_p->name.count);

		for (uint16_t i = 0; i < otf_p->name.count; ++i) {
			struct otf_name_rec_st* name_rec_p = otf_p->name.name_record_arr + i;
			serialization_read_uint16(&buf_p, &name_rec_p->platform_id);
			serialization_read_uint16(&buf_p, &name_rec_p->encoding_id);
			serialization_read_uint16(&buf_p, &name_rec_p->language_id);
			serialization_read_uint16(&buf_p, &name_rec_p->name_id);
			serialization_read_uint16(&buf_p, &name_rec_p->length);
			serialization_read_uint16(&buf_p, &name_rec_p->string_offset);
		}

		if (otf_p->name.version != 0) {
			serialization_read_uint16(&buf_p, &otf_p->name.lang_tag_count);

			otf_p->name.lang_tag_record_arr = malloc(sizeof(*otf_p->name.lang_tag_record_arr) * otf_p->name.lang_tag_count);

			for (uint16_t i = 0; i < otf_p->name.lang_tag_count; ++i) {
				struct otf_lang_tag_rec_st* lang_tag_record_p = otf_p->name.lang_tag_record_arr + i;
				serialization_read_uint16(&buf_p, &lang_tag_record_p->string_length);
				serialization_read_uint16(&buf_p, &lang_tag_record_p->string_offset);
			}
		} else
			otf_p->name.lang_tag_record_arr = 0;
		
		const size_t storage_buf_size = record_p->length - otf_p->name.storage_offset;
		otf_p->name.storage_buf_p = malloc(storage_buf_size);
		memcpy(otf_p->name.storage_buf_p, table_p + otf_p->name.storage_offset, storage_buf_size);

		otf_free_table(table_p);
	}

	// HHEA TABLE
	{
		record_p = otf_find_table_rec(table_record_arr, num_tables, OTF_TABLE_TAG_HHEA);
		table_p = otf_load_table(record_p, file_p);

		buf_p = table_p;

		serialization_read_uint16(&buf_p, &otf_p->hhea.version_major);
		serialization_read_uint16(&buf_p, &otf_p->hhea.version_minor);
		serialization_read_int16(&buf_p, &otf_p->hhea.ascender);
		serialization_read_int16(&buf_p, &otf_p->hhea.descender);
		serialization_read_int16(&buf_p, &otf_p->hhea.line_gap);
		serialization_read_uint16(&buf_p, &otf_p->hhea.advance_width_max);
		serialization_read_int16(&buf_p, &otf_p->hhea.min_left_side_bearing);
		serialization_read_int16(&buf_p, &otf_p->hhea.min_right_side_bearing);
		serialization_read_int16(&buf_p, &otf_p->hhea.x_max_extent);
		serialization_read_int16(&buf_p, &otf_p->hhea.caret_slope_rise);
		serialization_read_int16(&buf_p, &otf_p->hhea.caret_slope_run);
		serialization_read_int16(&buf_p, &otf_p->hhea.caret_offset);
		serialization_skip_bytes(&buf_p, 8);
		serialization_read_int16(&buf_p, &otf_p->hhea.metric_data_format);
		serialization_read_uint16(&buf_p, &otf_p->hhea.number_of_hmetrics);

		otf_free_table(table_p);
	}

	// MAXP TABLE
	{
		record_p = otf_find_table_rec(table_record_arr, num_tables, OTF_TABLE_TAG_MAXP);
		table_p = otf_load_table(record_p, file_p);
		
		buf_p = table_p;

		serialization_read_uint32(&buf_p, &otf_p->maxp.version_16dot16);
		serialization_read_uint16(&buf_p, &otf_p->maxp.num_glyphs);

		otf_free_table(table_p);
	}

	// HMTX TABLE
	{
		record_p = otf_find_table_rec(table_record_arr, num_tables, OTF_TABLE_TAG_HMTX);
		table_p = otf_load_table(record_p, file_p);
		
		buf_p = table_p;
		
		otf_p->hmtx.hmetric_arr = malloc(sizeof(*otf_p->hmtx.hmetric_arr) * otf_p->hhea.number_of_hmetrics);

		for (uint16_t i = 0; i < otf_p->hhea.number_of_hmetrics; ++i) {
			struct otf_long_hor_metric_st* long_hor_matric_p = otf_p->hmtx.hmetric_arr + i;
			serialization_read_uint16(&buf_p, &long_hor_matric_p->advance_width);
			serialization_read_int16(&buf_p, &long_hor_matric_p->left_side_bearing);
		}

		otf_p->hmtx.left_side_bearing_arr = malloc(sizeof(*otf_p->hmtx.left_side_bearing_arr) * otf_p->hhea.number_of_hmetrics);
		
		for (uint16_t i = 0; i < otf_p->hhea.number_of_hmetrics; ++i)
			serialization_read_int16(&buf_p, otf_p->hmtx.left_side_bearing_arr + i);

		otf_free_table(table_p);
	}

	// CMAP TABLE
	{
		record_p = otf_find_table_rec(table_record_arr, num_tables, OTF_TABLE_TAG_CMAP);
		table_p = otf_load_table(record_p, file_p);
		
		buf_p = table_p;
		
		serialization_read_uint16(&buf_p, &otf_p->cmap.version);
		serialization_read_uint16(&buf_p, &otf_p->cmap.encoding_record_arr_n);

		otf_p->cmap.encoding_record_arr = malloc(sizeof(*otf_p->cmap.encoding_record_arr) * otf_p->cmap.encoding_record_arr_n);

		for (uint16_t i = 0; i < otf_p->cmap.encoding_record_arr_n; ++i) {
			struct otf_encoding_rec_st* encoding_record_p = otf_p->cmap.encoding_record_arr + i;
			serialization_read_uint16(&buf_p, &encoding_record_p->platform_id);
			serialization_read_uint16(&buf_p, &encoding_record_p->encoding_id);
			serialization_read_uint32(&buf_p, &encoding_record_p->subtable_offset);
		}

		// todo: deal with encoding subtables

		otf_free_table(table_p);
	}

	// CFF TABLE
	{
		record_p = otf_find_table_rec(table_record_arr, num_tables, OTF_TABLE_TAG_CFF);
		table_p = otf_load_table(record_p, file_p);

		buf_p = table_p;

		struct otf_table_cff_st cff = {0};

		serialization_read_byte(&buf_p, &cff.version_major);
		serialization_read_byte(&buf_p, &cff.version_minor);
		serialization_read_byte(&buf_p, &cff.header_size);
		serialization_read_byte(&buf_p, &cff.offsets_size);

		buf_p = (unsigned char*)table_p + cff.header_size;
		
		struct otf_cff_index_st name_index;
		struct otf_cff_index_st top_dict_index;
		struct otf_cff_index_st string_index;
		struct otf_cff_index_st global_subr_index;

		OTF_DEBUG_LOG("Decoding Name INDEX...:\n");
		otf_cff_load_index(&buf_p, &name_index);

		OTF_DEBUG_LOG("Decoding Top DICT INDEX...:\n");
		otf_cff_load_index(&buf_p, &top_dict_index);

		OTF_DEBUG_LOG("Decoding String INDEX...:\n");
		otf_cff_load_index(&buf_p, &string_index);

		OTF_DEBUG_LOG("Decoding Global subr INDEX...:\n");
		otf_cff_load_index(&buf_p, &global_subr_index);

		for (uint16_t i = 0; i < top_dict_index.count; ++i) {
			void* top_dict_buf_p = ((unsigned char*)top_dict_index.data - 1) + top_dict_index.offset_arr[i];
			size_t top_dict_size = top_dict_index.offset_arr[i + 1] - top_dict_index.offset_arr[i];

			OTF_DEBUG_LOG("Decoding Top DICT...:\n");

			struct otf_cff_top_dict_st top_dict = {
				.char_string_type = 2
			};
			otf_cff_decode_dict(top_dict_buf_p, top_dict_size, otf_cff_handle_top_dict_operator, &top_dict);
			
			// Font DICT ? (CFF2?) CFF FontSets
			// FDArray ? (CFF2?) CFF FontSets
			
			OTF_DEBUG_LOG("Decoding Private DICT...\n");

			struct otf_cff_private_dict_st private_dict = {0};
			otf_cff_decode_dict((unsigned char*)table_p + top_dict.private_dict_offset, top_dict.private_dict_size, otf_cff_handle_private_dict_operator, &private_dict);

			OTF_DEBUG_LOG("Decoding Local subr INDEX...\n");

			buf_p = (unsigned char*)table_p + top_dict.private_dict_offset + private_dict.local_subr_offset;
			struct otf_cff_index_st local_subr_index;
			otf_cff_load_index(&buf_p, &local_subr_index);

			OTF_DEBUG_LOG("CharStrings INDEX:\n");

			buf_p = (unsigned char*)table_p + top_dict.char_strings_index_offset;
			struct otf_cff_index_st char_strings_index;
			otf_cff_load_index(&buf_p, &char_strings_index);

			for (uint16_t i = 0; i < char_strings_index.count; ++i) {
				void* char_string_p = ((unsigned char*)char_strings_index.data - 1) + char_strings_index.offset_arr[i];
				size_t char_string_length = char_strings_index.offset_arr[i + 1] - char_strings_index.offset_arr[i];

				// todo: decode glyphs from charstring

				// todo: decode Type 2 font numbers and operators

				// 1. Optional width number (difference from CFF nominalWidthX)
				// 2. hstem, hstemhm, vstem, vstemhm, cntrmask, hintmask
				// 3. Path construction
				// 4. endchar

				// Handle subr, gsubr

				OTF_DEBUG_LOG("---Begin CharString\n");

				struct cff_dict_number_st args[48];
				uint8_t argc = 0;

				uint32_t num_stems = 0;

				uint8_t* buf_p = char_string_p;
				while (buf_p) {;
					if (buf_p[0] == 28 || buf_p[0] >= 32) {
						args[argc] = (struct cff_dict_number_st){0};
						if (buf_p[0] == 28) {
							args[argc].value.as_int32 = *(int16_t*)(buf_p + 1);
							buf_p += 3;
						} else if (buf_p[0] <= 246) {
							args[argc].value.as_int32 = (int8_t)*buf_p - 139;
							buf_p += 1;
						} else if (buf_p[0] <= 250) {
							args[argc].value.as_int32 = ((int16_t)*buf_p - 247) * 256 + buf_p[1] + 108;
							buf_p += 2;
						} else if (buf_p[0] <= 254) {
							args[argc].value.as_int32 = -((int16_t)*buf_p - 251) * 256 - buf_p[1] - 108;
							buf_p += 2;
						} else {
							args[argc].value.as_int32 = *(int32_t*)buf_p;
							buf_p += 5;
						}
						argc++;
					} else {
						switch (buf_p[0]) {
							case 1: { // hstem
								num_stems += argc / 2;
								argc = 0;
								buf_p++;
								OTF_DEBUG_LOG("hstem\n");
								break;
							}

							case 18: { // hstemhm
								num_stems += argc / 2;
								argc = 0;
								buf_p++;
								OTF_DEBUG_LOG("hstemhm\n");
								break;
							}

							case 3: { // vstem
								num_stems += argc / 2;
								argc = 0;
								buf_p++;
								OTF_DEBUG_LOG("vstem\n");
								break;
							}

							case 23: { // vstemhm
								num_stems += argc / 2;
								argc = 0;
								buf_p++;
								OTF_DEBUG_LOG("vstemhm\n");
								break;
							}

							case 20: { // cntrmask
								num_stems += argc / 2;
								argc = 0;
								buf_p += (size_t)ceilf(num_stems / 8) + 1;
								OTF_DEBUG_LOG("cntrmask\n");
								break;
							}

							case 19: { // hintmask
								num_stems += argc / 2;
								argc = 0;
								buf_p += (size_t)ceilf(num_stems / 8) + 1;
								OTF_DEBUG_LOG("hintmask\n");
								break;
							}

							case 10: { // callsubr
								break;
							}

							case 29: { // callgsubr
								break;
							}

							case 12: { // two-byte operators
								switch (buf_p[1]) {
									default: {
										log_msg(LOG_DEBUG, OTF_LOG_CAT, "12 %hhu\n", buf_p[1]);
										break;
									}
								}
								buf_p += 2;
								break;
							}

							case 14: { // endchar
								buf_p = 0;
								OTF_DEBUG_LOG("---End CharString\n");
								break;
							}

							default: {
								log_msg(LOG_DEBUG, OTF_LOG_CAT, "%hhu\n", buf_p[0]);
								buf_p++;
								break;
							}
						}
					}
				}
			}

			otf_cff_free_index(&local_subr_index);
			otf_cff_free_index(&char_strings_index);
		}

		otf_cff_free_index(&name_index);
		otf_cff_free_index(&top_dict_index);
		otf_cff_free_index(&string_index);
		otf_cff_free_index(&global_subr_index);

		otf_free_table(table_p);
	}

	free(table_record_arr);
	
	fclose(file_p);

	for (uint16_t i = 0; i < otf_p->cmap.encoding_record_arr_n; ++i) {
		struct otf_encoding_rec_st* encoding_record_p = otf_p->cmap.encoding_record_arr + i;
		log_msg(LOG_DEBUG, OTF_LOG_CAT, "Encoding record[%hu]: PlatformID=%hu, EncodingID=%hu\n", i, encoding_record_p->platform_id, encoding_record_p->encoding_id);
	}
}

void otf_free(struct otf_st* otf_p) {
	free(otf_p->name.name_record_arr);
	free(otf_p->name.lang_tag_record_arr);
	free(otf_p->name.storage_buf_p);
	free(otf_p->hmtx.hmetric_arr);
	free(otf_p->hmtx.left_side_bearing_arr);
	free(otf_p->cmap.encoding_record_arr);
}