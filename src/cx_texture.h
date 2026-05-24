#ifndef CX_TEXTURE_H
#define CX_TEXTURE_H

#include "cx_asset.h"
#include "cx_gfx_texture.h"
#include "cx_pixel_format.h"
#include "cx_texture_sampler_settings.h"

#define ASSET_TYPE_TEXTURE 2

struct cx_texture {
    cx_asset_handle p_source_image;
    struct cx_texture_sampler_settings sampler_settings;
	int b_gfx_texture_loaded_;
	enum cx_pixel_format gfx_texture_format;
    struct cx_gfx_texture gfx_texture_;
};

void cx_texture_load_gfx_texture(struct cx_texture* p_texture, int b_force_load);
void cx_texture_unload_gfx_texture(struct cx_texture* p_texture);

#endif
