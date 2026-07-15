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

struct cx_stream;

void cx_texture_destroy(struct cx_texture* p_texture);
void cx_texture_load_gfx_texture(struct cx_texture* p_texture, int b_force_load);
void cx_texture_unload_gfx_texture(struct cx_texture* p_texture);
int cx_texture_serialize(const struct cx_texture* p_texture, struct cx_stream* p_stream);
int cx_texture_deserialize(struct cx_stream* p_stream, struct cx_texture* p_out_texture);

static inline void cx_texture_asset_destroy(void* p_asset) {
	cx_texture_destroy(p_asset);
}

static inline int cx_texture_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return cx_texture_serialize(p_asset, p_stream);
}

static inline int cx_texture_asset_deserialize(struct cx_stream* p_stream, void* p_out_asset) {
	return cx_texture_deserialize(p_stream, p_out_asset);
}

#endif
