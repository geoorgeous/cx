#include "cx_gfx_framebuffer.h"
#include "cx_gfx_texture.h"
#include "cx_gfx_texture.gl.h"
#include "cx_error.h"
#include "gl.h"

static const GLenum gl_fb_attachment_point_table[] = {
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5,
	GL_COLOR_ATTACHMENT6,
	GL_COLOR_ATTACHMENT7,
	GL_DEPTH_ATTACHMENT,
	GL_STENCIL_ATTACHMENT,
	GL_DEPTH_STENCIL_ATTACHMENT
};

struct cx_gfx_framebuffer_gl_internals {
	GLuint id;
};

enum cx_error cx_gfx_framebuffer_create(struct cx_gfx_framebuffer* p_framebuffer) {
	struct cx_gfx_framebuffer_gl_internals* p_internals = (void*)p_framebuffer->bytes_;

	glGenFramebuffers(1, &p_internals->id);

	if (!p_internals->id) {
		return CX_ERROR_allocation_failed;
	}

	return CX_ERROR_none;
}

void cx_gfx_framebuffer_destroy(struct cx_gfx_framebuffer* p_framebuffer) {
	struct cx_gfx_framebuffer_gl_internals* p_internals = (void*)p_framebuffer->bytes_;

	glDeleteFramebuffers(1, &p_internals->id);

	*p_framebuffer = (struct cx_gfx_framebuffer){0};
}

void cx_gfx_framebuffer_set_attachment(
	const struct cx_gfx_framebuffer* p_framebuffer,
	enum cx_gfx_framebuffer_attachment attachment_point,
	const struct cx_gfx_texture* p_texture) {

	const struct cx_gfx_framebuffer_gl_internals* p_internals = (const void*)p_framebuffer->bytes_;
	const struct cx_gfx_texture_gl_internals* p_texture_internals = (const void*)p_texture->bytes_;

    glBindFramebuffer(GL_FRAMEBUFFER, p_internals->id);
    
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		gl_fb_attachment_point_table[attachment_point],
		GL_TEXTURE_2D,
		p_texture_internals->id,
		0);
}

void cx_gfx_framebuffer_bind(const struct cx_gfx_framebuffer *p_framebuffer) {
	const struct cx_gfx_framebuffer_gl_internals* p_internals = (const void*)p_framebuffer;

	glBindFramebuffer(GL_FRAMEBUFFER, p_internals->id);
}

void cx_gfx_framebuffer_read(
	const struct cx_gfx_framebuffer* p_framebuffer,
	enum cx_gfx_framebuffer_attachment attachment,
	const uint32_t* p_read_position,
	const uint32_t* p_read_size,
	void* p_out_read_buffer) {

	const struct cx_gfx_framebuffer_gl_internals* p_internals = (const void*)p_framebuffer->bytes_;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, p_internals->id);
    glReadBuffer(gl_fb_attachment_point_table[attachment]);

    glReadPixels(
		(GLint)p_read_position[0],
		(GLint)p_read_position[1],
		(GLsizei)p_read_size[0],
		(GLsizei)p_read_size[1], 
		GL_RED_INTEGER,
		GL_UNSIGNED_INT,
		p_out_read_buffer);
}
