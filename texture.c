#include "gl_texture.h"
#include "texture.h"

void texture_load_device_texture(struct texture* p_texture) {
    const struct image* p_image = p_texture->p_source_image->_asset._p_data;
    gl_texture_create(&p_texture->gl_texture, p_image, &p_texture->sampler);
}

void texture_unload_device_texture(struct texture* p_texture) {
    gl_texture_destroy(&p_texture->gl_texture);
}