#ifndef _H__CX_PIXEL_FORMAT
#define _H__CX_PIXEL_FORMAT

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

#endif
