#ifndef _H__FONT
#define _H__FONT

#include <stdint.h>

#include "darr.h"
#include "hashtable.h"

#define FONT_STRING_MAX_LEN 70

struct glyph_metrics {
    size_t code;
    int    scalable_size[2];
    int    device_size[2];
    int    size[2];
    int    offset[2];
};

struct glyph {
    struct glyph_metrics metrics;
    void*                p_bitmap;
};

struct font_face_metrics {
    int point_size;
    int device_size[2];
    int glyph_size[2];
    int glyph_offset[2];
};

enum font_style {
    FONT_STYLE_regular,
    FONT_STYLE_bold,
    FONT_STYLE_italic,
    FONT_STYLE_bold_italic
};

struct font_face {
    char                     s_name[FONT_STRING_MAX_LEN];
    enum font_style          style;
    struct font_face_metrics metrics;
    struct hashtable         glyphs;
};

void font_face_generate_glyph_texture_atlas(const struct font_face* p_font_face, struct glyph_texture_atlas* p_result);

struct glyph_texture_atlas_entry {
    const struct glyph_st* glyph;
    float uv_min_u;
    float uv_min_v;
    float uv_max_u;
    float uv_max_v;
};

struct glyph_texture_atlas {
    const struct font_face* p_source_font_face;
    size_t                  pixels_width;
    size_t                  pixels_height;
    void*                   p_pixels;
    struct hashtable        glyph_entries;
};

struct font_family {
    char             s_name[FONT_STRING_MAX_LEN];
    struct darr      faces;
};

#endif