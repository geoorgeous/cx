#ifndef _H__GL_TEXTURE
#define _H__GL_TEXTURE

#include "gl.h"

struct image;
struct texture_sampler;

struct gl_texture {
    GLuint gl_handle;
};

void gl_texture_create(struct gl_texture* p_gl_texture, const struct image* p_image, const struct texture_sampler* p_sampler);
void gl_texture_destroy(struct gl_texture* p_gl_texture);

#endif