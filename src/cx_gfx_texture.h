#ifndef CX_GFX_TEXTURE_H
#define CX_GFX_TEXTURE_H

#include <stdint.h>

#include "cx_error.h"
#include "cx_pixel_format.h"

#define CX_LOG_CAT_GFX_TEXTURE "gfx:texture"

struct cx_gfx_texture {
	uint32_t             width_;
	uint32_t             height_;
	enum cx_pixel_format pixel_format_;
	char                 bytes_[8];
};

enum cx_error cx_gfx_texture_create(
	struct cx_gfx_texture* p_texture,
	uint32_t width, uint32_t height,
	enum cx_pixel_format pixel_format);

void cx_gfx_texture_destroy(struct cx_gfx_texture* p_texture);

void cx_gfx_texture_set_data(
	const struct cx_gfx_texture* p_texture,
	const void* p_data,
	const struct cx_pixel_buffer_format* p_data_format);

void cx_gfx_texture_set_data_subregion(
	const struct cx_gfx_texture* p_texture,
	const void* p_data,
	const struct cx_pixel_buffer_format* p_format,
	uint32_t offset_x, uint32_t offset_y,
	uint32_t width, uint32_t height);

struct cx_texture_sampler_settings;

void cx_gfx_texture_set_sampler_settings(
	struct cx_gfx_texture* p_texture,
	const struct cx_texture_sampler_settings* p_sampler_settings);

#endif
