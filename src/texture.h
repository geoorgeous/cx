#ifndef _H__TEXTURE
#define _H__TEXTURE

#include "asset.h"
#include "gl_texture.h"
#include "texture_sampler.h"

#define ASSET_TYPE_TEXTURE 2

struct image;

struct texture {
    asset_handle            p_source_image;
    struct texture_sampler  sampler;
    struct gl_texture       gl_texture;
};

void texture_load_device_texture(struct texture* p_texture);
void texture_unload_device_texture(struct texture* p_texture);

#endif