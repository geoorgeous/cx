#ifndef CX_GFX_TEXTURE_H
#define CX_GFX_TEXTURE_H

#include <stdint.h>

#include "errors.h"
#include "cx_pixel_format.h"

#define CX_LOG_CAT_GFX_TEXTURE "gfx:texture"

struct cx_gfx_texture {
	uint32_t             _size[2];
	enum cx_pixel_format _pixel_format;
	char                 _bytes[8];
};

enum error cx_gfx_texture_create(
	struct cx_gfx_texture* p_texture,
	const uint32_t* p_size,
	enum cx_pixel_format pixel_format);

void       cx_gfx_texture_destroy(struct cx_gfx_texture* p_texture);

void       cx_gfx_texture_set_data(
	const struct cx_gfx_texture* p_texture,
	const void* p_data,
	const struct cx_pixel_buffer_format* p_data_format);

void       cx_gfx_texture_set_data_subregion(
	const struct cx_gfx_texture* p_texture,
	const void* p_data,
	const struct cx_pixel_buffer_format* p_format,
	const uint32_t* region_offset,
	const uint32_t* region_size);

struct cx_texture_sampler_settings;

void       cx_gfx_texture_set_sampler_settings(
	struct cx_gfx_texture* p_texture,
	const struct cx_texture_sampler_settings* p_sampler_settings);

#endif
