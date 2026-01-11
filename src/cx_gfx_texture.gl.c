#include <stdint.h>

#include "cx_gfx_texture.h"
#include "cx_gfx_texture.gl.h"
#include "cx_pixel_format.h"
#include "cx_texture_sampler_settings.h"
#include "errors.h"
#include "gl.h"
#include "logging.h"

static const GLenum gl_pixel_format_table[] = {
	GL_RED,
	GL_RG,
	GL_RGB,
	GL_RGBA,
	GL_BGR,
	GL_BGRA,
	GL_DEPTH24_STENCIL8,
	GL_R32UI
};

static const GLenum gl_pixel_type_table[] = {
	GL_UNSIGNED_BYTE,
	GL_BYTE,
	GL_UNSIGNED_SHORT,
	GL_SHORT,
	GL_UNSIGNED_INT,
	GL_INT
};

static const GLenum gl_filter_mode_table[] = {
	GL_NEAREST,
	GL_LINEAR,
	GL_NEAREST_MIPMAP_NEAREST,
	GL_LINEAR_MIPMAP_NEAREST,
	GL_NEAREST_MIPMAP_LINEAR,
	GL_LINEAR_MIPMAP_LINEAR
};

static const GLenum gl_address_mode_table[] = {
	GL_REPEAT,
	GL_MIRRORED_REPEAT,
	GL_CLAMP_TO_EDGE
};

void get_valid_pixel_transfer_params_for_format(
	GLenum internal_format,
	GLenum* p_out_pixel_format,
	GLenum* p_out_pixel_type);

enum error cx_gfx_texture_create(
	struct cx_gfx_texture* p_texture,
	const uint32_t* p_size,
	enum cx_pixel_format pixel_format) {
    
	struct cx_gfx_texture_gl_internals* p_internals = (void*)p_texture->_bytes;

	GLuint id;

    glGenTextures(1, &id);

    glBindTexture(GL_TEXTURE_2D, id);

	const GLenum gl_internal_format = gl_pixel_format_table[pixel_format];
	
	GLenum gl_pixel_format;
	GLenum gl_pixel_type;
	get_valid_pixel_transfer_params_for_format(gl_internal_format, &gl_pixel_format, &gl_pixel_type);

    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, p_size[0], p_size[1], 0, gl_pixel_format, gl_pixel_type, 0);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	cx_log_fmt(
		CX_LOG_INFO,
		CX_LOG_CAT_TEXTURE,
		"Texture created: (%ux%u) internal_format=%x, pixel_format=0x%x, pixel_type=0x%x\n",
		p_size[0],
		p_size[1],
		gl_internal_format,
		gl_pixel_format,
		gl_pixel_type);

	*p_texture = (struct cx_gfx_texture) {
		._size = { p_size[0], p_size[1] },
		._pixel_format = pixel_format
	};

	*p_internals = (struct cx_gfx_texture_gl_internals) {
		.id = id
	};

	return ERROR_OK;
}

void cx_gfx_texture_destroy(struct cx_gfx_texture* p_texture) {
	struct cx_gfx_texture_gl_internals* p_internals = (void*)p_texture->_bytes;
	glDeleteTextures(1, &p_internals->id);
	*p_texture = (struct cx_gfx_texture){0};
}

void cx_gfx_texture_set_data(
	const struct cx_gfx_texture* p_texture,
	const void* p_data,
	const struct cx_pixel_buffer_format* p_format) {
	
	cx_gfx_texture_set_data_subregion(
		p_texture,
		p_data,
		p_format,
		(uint32_t[]){ 0, 0 },
		p_texture->_size);
}

void cx_gfx_texture_set_data_subregion(
	const struct cx_gfx_texture* p_texture,
	const void* p_data,
	const struct cx_pixel_buffer_format* p_data_format,
	const uint32_t* region_offset,
	const uint32_t* region_size) {

	const struct cx_gfx_texture_gl_internals* p_internals = (const void*)p_texture->_bytes;

	glBindTexture(GL_TEXTURE_2D, p_internals->id);

	glTexSubImage2D(
		GL_TEXTURE_2D,
		0,
		(GLint)region_offset[0],
		(GLint)region_offset[1],
		(GLint)region_size[0],
		(GLint)region_size[1],
		gl_pixel_format_table[p_data_format->pixel_format],
		gl_pixel_type_table[p_data_format->pixel_type],
		p_data);

	if (p_internals->b_mipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void cx_gfx_texture_set_sampler_settings(
	struct cx_gfx_texture* p_texture,
	const struct cx_texture_sampler_settings* p_sampler_settings) {

	struct cx_gfx_texture_gl_internals* p_internals = (void*)p_texture->_bytes;

	glBindTexture(GL_TEXTURE_2D, p_internals->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,  gl_filter_mode_table[p_sampler_settings->min_filter_mode]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_mode_table[p_sampler_settings->mag_filter_mode]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_address_mode_table[p_sampler_settings->address_mode_u]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_address_mode_table[p_sampler_settings->address_mode_v]);

	p_internals->b_mipmaps =
		gl_filter_mode_table[p_sampler_settings->min_filter_mode] != GL_NEAREST &&
		gl_filter_mode_table[p_sampler_settings->min_filter_mode] != GL_LINEAR;

	if (p_internals->b_mipmaps) {
		glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void get_valid_pixel_transfer_params_for_format(
	GLenum internal_format,
	GLenum* p_out_pixel_format,
	GLenum* p_out_pixel_type) {
	
	switch(internal_format) {
		case GL_DEPTH24_STENCIL8: {
			*p_out_pixel_format = GL_DEPTH_STENCIL;
			*p_out_pixel_type = GL_UNSIGNED_INT_24_8;
			break;
		}

		case GL_R32UI: {
			*p_out_pixel_format = GL_RED_INTEGER;
			*p_out_pixel_type = GL_UNSIGNED_INT;
			break;
		}

		default: {
			*p_out_pixel_format = internal_format;
			*p_out_pixel_type = GL_UNSIGNED_BYTE;
			break;
		}
	}
}
