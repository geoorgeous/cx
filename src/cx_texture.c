#include "cx_image.h"
#include "cx_texture.h"
#include "cx_gfx_texture.h"

void cx_texture_load_gfx_texture(struct cx_texture* p_texture, int b_force_reload) {
	if (p_texture->b_loaded_) {
		if (!b_force_reload) {
			return;
		}

		cx_texture_unload_gfx_texture(p_texture);
	}

    const struct cx_image* p_image = p_texture->p_source_image->asset_.p_data_;
	cx_gfx_texture_create(&p_texture->gfx_texture_, p_image->width, p_image->height, p_texture->gfx_texture_format);
	cx_gfx_texture_set_sampler_settings(&p_texture->gfx_texture_, &p_texture->sampler_settings);
	cx_gfx_texture_set_data(&p_texture->gfx_texture_, p_image->p_pixel_data, &p_image->pixel_data_format);

	p_texture->b_loaded_ = 1;
}

void cx_texture_unload_gfx_texture(struct cx_texture* p_texture) {
	cx_gfx_texture_destroy(&p_texture->gfx_texture_);
	p_texture->b_loaded_ = 0;
}
