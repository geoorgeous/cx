#include "gl_texture.h"
#include "image.h"
#include "texture_sampler.h"

static GLint texture_filter_min_to_glenum(enum texture_filter_min filter_min);
static GLint texture_filter_mag_to_glenum(enum texture_filter_mag filter_mag);

void gl_texture_create(struct gl_texture* p_gl_texture, const struct image* p_image, const struct texture_sampler* p_sampler) {
    const GLenum gl_texture_target = GL_TEXTURE_2D;

    glGenTextures(1, &p_gl_texture->gl_handle);
    glBindTexture(gl_texture_target, p_gl_texture->gl_handle);
    glTexImage2D(gl_texture_target, 0, GL_RGB, p_image->width, p_image->height, 0, GL_RGB, GL_UNSIGNED_BYTE, p_image->p_pixel_data);

    GLint gl_filter_min = GL_LINEAR;
    GLint gl_filter_mag = GL_LINEAR;
    
    if (p_sampler) {
        gl_filter_min = texture_filter_min_to_glenum(p_sampler->filter_min);
        gl_filter_mag = texture_filter_mag_to_glenum(p_sampler->filter_mag);
        
        if (gl_filter_min != GL_NEAREST && gl_filter_min != GL_LINEAR) {
            glGenerateMipmap(gl_texture_target);
        }
    }

    glTexParameteri(gl_texture_target, GL_TEXTURE_MIN_FILTER, gl_filter_min);
    glTexParameteri(gl_texture_target, GL_TEXTURE_MAG_FILTER, gl_filter_mag);
}

void gl_texture_destroy(struct gl_texture* p_gl_texture) {
    glDeleteTextures(1, &p_gl_texture->gl_handle);
}

GLint texture_filter_min_to_glenum(enum texture_filter_min filter_min) {
    switch(filter_min) {
        case TEXTURE_FILTER_MIN_nearest:                return GL_NEAREST;
        case TEXTURE_FILTER_MIN_linear:                 return GL_LINEAR;
        case TEXTURE_FILTER_MIN_nearest_mipmap_nearest: return GL_NEAREST_MIPMAP_NEAREST;
        case TEXTURE_FILTER_MIN_linear_mipmap_nearest:  return GL_LINEAR_MIPMAP_NEAREST;
        case TEXTURE_FILTER_MIN_nearest_mipmap_linear:  return GL_NEAREST_MIPMAP_LINEAR;
        case TEXTURE_FILTER_MIN_linear_mipmap_linear:   return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_NONE;
}

GLint texture_filter_mag_to_glenum(enum texture_filter_mag filter_mag) {
    switch(filter_mag) {
        case TEXTURE_FILTER_MAG_nearest: return GL_NEAREST;
        case TEXTURE_FILTER_MAG_linear:  return GL_LINEAR;
    }
    return GL_NONE;
}