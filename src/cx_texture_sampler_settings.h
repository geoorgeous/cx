#ifndef _H__CX_TEXTURE_SAMPLER_SETTINGS
#define _H__CX_TEXTURE_SAMPLER_SETTINGS

enum cx_texture_mag_filter_mode {
	CX_TEXTURE_MAG_FILTER_MODE_nearest,
	CX_TEXTURE_MAG_FILTER_MODE_linear,
};

enum cx_texture_min_filter_mode {
	CX_TEXTURE_MIN_FILTER_MODE_nearest,
	CX_TEXTURE_MIN_FILTER_MODE_linear,
	CX_TEXTURE_MIN_FILTER_MODE_nearest_mipmap_nearest,
	CX_TEXTURE_MIN_FILTER_MODE_linear_mipmap_nearest,
	CX_TEXTURE_MIN_FILTER_MODE_nearest_mipmap_linear,
	CX_TEXTURE_MIN_FILTER_MODE_linear_mipmap_linear
};

enum cx_texture_address_mode {
	CX_TEXTURE_ADDRESS_MODE_repeat,
	CX_TEXTURE_ADDRESS_MODE_mirrored_repeat,
	CX_TEXTURE_ADDRESS_MODE_clamp_to_edge
};

struct cx_texture_sampler_settings {
	enum cx_texture_mag_filter_mode mag_filter_mode;
	enum cx_texture_min_filter_mode min_filter_mode;
	enum cx_texture_address_mode    address_mode_u;
	enum cx_texture_address_mode    address_mode_v;
};

#endif
