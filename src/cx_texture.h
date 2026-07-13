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

struct cx_stream_writer;
struct cx_stream_reader;

void cx_texture_destroy(struct cx_texture* p_texture);
void cx_texture_load_gfx_texture(struct cx_texture* p_texture, int b_force_load);
void cx_texture_unload_gfx_texture(struct cx_texture* p_texture);
int cx_texture_serialize(const struct cx_texture* p_texture, struct cx_stream_writer* p_writer);
int cx_texture_deserialize(struct cx_texture* p_texture, struct cx_stream_reader* p_reader);

static inline void cx_texture_asset_destroy(void* p) {
	cx_texture_destroy(p);
}

static inline int cx_texture_asset_serialize(struct cx_stream_writer* p_writer, const void* p) {
	return cx_texture_serialize(p, p_writer);
}

static inline int cx_texture_asset_deserialize(struct cx_stream_reader* p_reader, void* p) {
	return cx_texture_deserialize(p, p_reader);
}

#endif
