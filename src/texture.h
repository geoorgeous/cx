#ifndef TEXTURE_H
#define TEXTURE_H

#include "asset.h"
#include "cx_gfx_texture.h"
#include "cx_pixel_format.h"
#include "cx_texture_sampler_settings.h"

#define ASSET_TYPE_TEXTURE 2

struct texture {
    asset_handle                       p_source_image;
    struct cx_texture_sampler_settings sampler_settings;
	int                                b_loaded_;
	enum   cx_pixel_format             gfx_texture_format;
    struct cx_gfx_texture              gfx_texture_;
};

void texture_load_gfx_texture(struct texture* p_texture, int b_force_load);
void texture_unload_gfx_texture(struct texture* p_texture);

#endif
