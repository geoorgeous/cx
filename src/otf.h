#ifndef _H__OTF
#define _H__OTF

#include <stdint.h>

struct otf_table_rec_st {
	uint8_t  tag[5];                 /* Table identifier. */
	uint32_t checksum;               /* Checksum for this table. */
	uint32_t offset;                 /* Offset from beginning of font file. */
	uint32_t length;                 /* Length of this table. */
};

struct otf_encoding_rec_st {
	uint16_t platform_id;            /* Platform ID. */
	uint16_t encoding_id;            /* Platform-specific encoding ID. */
	uint32_t subtable_offset;        /* Byte offset from beginning of table to the subtable for this encoding. */
};

struct otf_name_rec_st {
	uint16_t platform_id;            /* Platform ID. */
	uint16_t encoding_id;            /* Platform-specific encoding ID. */
	uint16_t language_id;            /* Language ID. */
	uint16_t name_id;                /* Name ID. */
	uint16_t length;                 /* String length (in bytes). */
	uint16_t string_offset;          /* String offset from start of storage area (in bytes). */
};

struct otf_lang_tag_rec_st {
	uint16_t string_length;          /* Language-tag string length (in bytes). */
	uint16_t string_offset;          /* Language-tag string offset from start of storage area (in bytes). */
};

struct otf_long_hor_metric_st {
	uint16_t advance_width;          /* Advance width, in font design units. */
	int16_t  left_side_bearing;      /* Glyph left side bearing, in font design units. */
};

struct otf_table_head_st {
	uint16_t version_major;          /* Major version number of the font header table — set to 1. */
	uint16_t version_minor;          /* Minor version number of the font header table — set to 0. */
	uint32_t revision_number_fp;     /* Set by font manufacturer. */
	uint32_t checksum_adjustment;    /* To compute: set it to 0, sum the entire font as uint32, then store 0xB1B0AFBA -
	                                    sum. If the font is used as a component in a font collection file, the value of
	                                    this field will be invalidated by changes to the file structure and font table
	                                    directory, and must be ignored. */
	uint32_t magix_number;           /* Set to 0x5F0F3CF5. */
	uint16_t flags;
	uint16_t units_per_em;           /* Set to a value from 16 to 16384. Any value in this range is valid. In fonts that
                                        have TrueType outlines, a power of 2 is recommended as this allows performance
	                                    optimization in some rasterizers. */
	int64_t  datetime_created;       /* Number of seconds since 12:00 midnight that started January 1st, 1904, in
	                                    GMT/UTC time zone. */
	int64_t  datetime_modified;      /* Number of seconds since 12:00 midnight that started January 1st, 1904, in
	                                    GMT/UTC time zone. */
	int16_t  x_min;                  /* Minimum x coordinate across all glyph bounding boxes. */
	int16_t  y_min;                  /* Minimum y coordinate across all glyph bounding boxes. */
	int16_t  x_max;                  /* Maximum x coordinate across all glyph bounding boxes. */
	int16_t  y_max;                  /* Maximum y coordinate across all glyph bounding boxes. */
	uint16_t mac_style;
	uint16_t lowest_rec_ppem;        /* Smallest readable size in pixels. (Lowest Recommended Pixels Per EM)*/
	int16_t  font_direction_hint;    /* Deprecated (Set to 2). */
	int16_t  index_to_loc_format;    /* 0 for short offsets (Offset16), 1 for long (Offset32). */
	int16_t  glyph_data_format;      /* 0 for current format. */
};

struct otf_table_name_st {
	uint16_t version;                /* Table version number (=0). */
	uint16_t count;                  /* Number of name records. */
	uint16_t storage_offset;         /* Offset to start of string storage (from start of table). */
	struct otf_name_rec_st* name_record_arr; /* The name records where count is the number of records. */
	uint16_t lang_tag_count;         /* Number of language-tag records. */
	struct otf_lang_tag_rec_st* lang_tag_record_arr; /* The language-tag records where langTagCount is the number of
	                                                    records. */
	void*    storage_buf_p;          /* Storage for the actual string data. */
};

struct otf_table_hhea_st {
	uint16_t version_major;          /* Major version number of the horizontal header table — set to 1. */
	uint16_t version_minor;          /* Minor version number of the horizontal header table — set to 0. */
	int16_t  ascender;               /* Typographic ascent—see remarks below. */
	int16_t  descender;              /* Typographic descent—see remarks below. */
	int16_t  line_gap;               /* Typographic line gap. Negative lineGap values are treated as zero in some legacy
	                                    platform implementations. */
	uint16_t advance_width_max;      /* Maximum advance width value in 'hmtx' table. */
	int16_t  min_left_side_bearing;  /* Minimum left sidebearing value in 'hmtx' table for glyphs with contours (empty
	                                    glyphs should be ignored). */
	int16_t  min_right_side_bearing; /* Minimum right sidebearing value; calculated as min(aw - (lsb + xMax - xMin)) for
	                                    glyphs with contours (empty glyphs should be ignored). */
	int16_t  x_max_extent;           /* Max(lsb + (xMax - xMin)). */
	int16_t  caret_slope_rise;       /* Used to calculate the slope of the cursor (rise/run); 1 for vertical. */
	int16_t  caret_slope_run;        /* 0 for vertical. */
	int16_t  caret_offset;           /* The amount by which a slanted highlight on a glyph needs to be shifted to
	                                    produce the best appearance. Set to 0 for non-slanted fonts */
	int16_t  metric_data_format;     /* 0 for current format. */
	uint16_t number_of_hmetrics;     /* Number of hMetric entries in 'hmtx' table */
};

struct otf_table_maxp_st {
	uint32_t version_16dot16;        /* 0x00005000 for version 0.5 */
	uint16_t num_glyphs;             /* The number of glyphs in the font. */
};

struct otf_table_hmtx_st {
	struct otf_long_hor_metric_st* hmetric_arr; /* Paired advance width and left side bearing values for each glyph.
	                                               Records are indexed by glyph ID. */
	int16_t* left_side_bearing_arr;  /* Left side bearings for glyph IDs greater than or equal to numberOfHMetrics. */
};

struct otf_table_cmap_st {
	uint16_t version;                /* Table version number (0). */
	uint16_t encoding_record_arr_n;  /* Number of encoding tables. */
	struct otf_encoding_rec_st *encoding_record_arr;
};

struct otf_table_os_2_st {

};

struct otf_table_cff_st {
	uint8_t version_major;           /* Format major version (starting at 1) */
	uint8_t version_minor;           /* Format minor version (starting at 0) */
	uint8_t header_size;             /* Header size (bytes) */
	uint8_t offsets_size;            /* Absolute offset (0) size */
};

struct otf_st {
	struct otf_table_head_st head;
	struct otf_table_name_st name;
	struct otf_table_hhea_st hhea;
	struct otf_table_maxp_st maxp;
	struct otf_table_hmtx_st hmtx;
	struct otf_table_cmap_st cmap;
};

void otf_load(const char* filepath_str, struct otf_st* otf_p);
void otf_free(struct otf_st* otf_p);

#endif