#include "cx_asset_cache.h"
#include "cx_gfx_texture.h"
#include "cx_image.h"
#include "cx_macro.h"
#include "cx_stream_serialization.h"
#include "cx_texture.h"

void cx_texture_destroy(struct cx_texture* p_texture) {
	cx_texture_unload_gfx_texture(p_texture);
}

void cx_texture_load_gfx_texture(struct cx_texture* p_texture, int b_force_reload) {
	if (p_texture->b_gfx_texture_loaded_) {
		if (!b_force_reload) {
			return;
		}

		cx_texture_unload_gfx_texture(p_texture);
	}

	const struct cx_image* p_image = cx_asset_cache_acquire(&p_texture->source_image_asset_ref);
	
	cx_gfx_texture_create(&p_texture->gfx_texture_, p_image->width, p_image->height, p_texture->gfx_texture_format);
	cx_gfx_texture_set_sampler_settings(&p_texture->gfx_texture_, &p_texture->sampler_settings);
	cx_gfx_texture_set_data(&p_texture->gfx_texture_, p_image->p_pixel_data, &p_image->pixel_data_format);
	
	cx_asset_cache_release(&p_texture->source_image_asset_ref);

	p_texture->b_gfx_texture_loaded_ = 1;
}

void cx_texture_unload_gfx_texture(struct cx_texture* p_texture) {
	cx_gfx_texture_destroy(&p_texture->gfx_texture_);
	p_texture->b_gfx_texture_loaded_ = 0;
}

int cx_texture_serialize(const struct cx_texture* p_texture, struct cx_stream* p_stream) {
	cx_asset_ref_serialize(&p_texture->source_image_asset_ref, p_stream);
	
	cx_stream_serialize_uint8(p_stream, (uint8_t)p_texture->sampler_settings.mag_filter_mode);
	cx_stream_serialize_uint8(p_stream, (uint8_t)p_texture->sampler_settings.min_filter_mode);
	cx_stream_serialize_uint8(p_stream, (uint8_t)p_texture->sampler_settings.address_mode_u);
	cx_stream_serialize_uint8(p_stream, (uint8_t)p_texture->sampler_settings.address_mode_v);
	cx_stream_serialize_uint8(p_stream, (uint8_t)p_texture->gfx_texture_format);

	return CX_TRUE;
}

int cx_texture_deserialize(struct cx_stream* p_stream, struct cx_texture* p_out_texture) {
	cx_asset_ref_deserialize(p_stream, &p_out_texture->source_image_asset_ref);
	
	uint8_t temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_out_texture->sampler_settings.mag_filter_mode = (enum cx_texture_mag_filter_mode)temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_out_texture->sampler_settings.min_filter_mode = (enum cx_texture_min_filter_mode)temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_out_texture->sampler_settings.address_mode_u = (enum cx_texture_address_mode)temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_out_texture->sampler_settings.address_mode_v = (enum cx_texture_address_mode)temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_out_texture->gfx_texture_format = (enum cx_pixel_format)temp;

	return CX_TRUE;
}
