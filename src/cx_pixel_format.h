#ifndef CX_PIXEL_FORMAT_H
#define CX_PIXEL_FORMAT_H

#include <stddef.h>

enum cx_pixel_format {
	CX_PIXEL_FORMAT_red,
	CX_PIXEL_FORMAT_rg,
	CX_PIXEL_FORMAT_rgb,
	CX_PIXEL_FORMAT_rgba,
	CX_PIXEL_FORMAT_bgr,
	CX_PIXEL_FORMAT_bgra,
	CX_PIXEL_FORMAT_depth_stencil,
	CX_PIXEL_FORMAT_red_u32
};

enum cx_pixel_type {
	CX_PIXEL_TYPE_u8,
	CX_PIXEL_TYPE_i8,
	CX_PIXEL_TYPE_u16,
	CX_PIXEL_TYPE_i16,
	CX_PIXEL_TYPE_u32,
	CX_PIXEL_TYPE_i32
};

struct cx_pixel_buffer_format {
	enum cx_pixel_format pixel_format;
	enum cx_pixel_type   pixel_type;
};

static inline unsigned char cx_pixel_format_num_components(enum cx_pixel_format format) {
	static const unsigned char format_num_components_table[] = { 1, 2, 3, 4, 3, 4, 1, 1 };
	return format_num_components_table[format];
}

static inline size_t cx_pixel_type_size(enum cx_pixel_type type) {
	static const size_t type_size_table[] = { 1, 1, 2, 2, 3, 3 };
	return type_size_table[type];
}

static inline size_t cx_pixel_buffer_format_compute_pixel_size(const struct cx_pixel_buffer_format* p_format) {
	return cx_pixel_format_num_components(p_format->pixel_format) * cx_pixel_type_size(p_format->pixel_type);
}

#endif
