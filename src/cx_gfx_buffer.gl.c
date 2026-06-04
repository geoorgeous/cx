#include "cx_dbg.h"
#include "cx_gfx_buffer.h"
#include "cx_gfx_buffer.gl.h"

void cx_gfx_buffer_create(
	enum cx_gfx_buffer_type type,
	size_t size,
	enum cx_gfx_buffer_usage usage,
	struct cx_gfx_buffer* p_out) {

	GLuint gl_id;
	glGenBuffers(1, &gl_id);

	GLenum gl_target = GL_NONE;
	switch(type) {
		case CX_GFX_BUFFER_TYPE_vertex: gl_target = GL_ARRAY_BUFFER; break;
		case CX_GFX_BUFFER_TYPE_index: gl_target = GL_ELEMENT_ARRAY_BUFFER; break;
		case CX_GFX_BUFFER_TYPE_uniform: gl_target = GL_UNIFORM_BUFFER; break;
	}

	GLenum gl_usage = GL_NONE;
	switch(usage) {
		case CX_GFX_BUFFER_USAGE_static: gl_usage = GL_STATIC_DRAW; break;
		case CX_GFX_BUFFER_USAGE_dynamic: gl_usage = GL_DYNAMIC_DRAW; break;
	}

	CX_ASSERT(gl_target != GL_NONE, GFX_BUFFER);
	CX_ASSERT(gl_usage != GL_NONE, GFX_BUFFER);

	glBindBuffer(gl_target, gl_id);
	glBufferData(gl_target, (GLsizeiptr)size, CX_NULL, gl_usage);

	*p_out = (struct cx_gfx_buffer) {
		.type_ = type,
		.usage_ = usage,
		.size_ = size
	};

	struct cx_gfx_buffer_gl_internals* p_internals = (void*)p_out->internals_.bytes_;

	*p_internals = (struct cx_gfx_buffer_gl_internals) {
		.id = gl_id,
		.target = gl_target,
		.usage = gl_usage
	};
}

void cx_gfx_buffer_destroy(struct cx_gfx_buffer* p_buffer) {
	struct cx_gfx_buffer_gl_internals* p_internals = (void*)p_buffer->internals_.bytes_;
	glDeleteBuffers(1, &p_internals->id);
	*p_buffer = (struct cx_gfx_buffer) {0};
}

void cx_gfx_buffer_set(struct cx_gfx_buffer* p_buffer, size_t size, const void* p_data) {
	p_buffer->size_ = size ? size : p_buffer->size_;
	const struct cx_gfx_buffer_gl_internals* p_internals = (const void*)p_buffer->internals_.bytes_;
	glBindBuffer(p_internals->target, p_internals->id);
	glBufferData(p_internals->target, (GLsizeiptr)p_buffer->size_, p_data, p_internals->usage);
}

void cx_gfx_buffer_set_region(const struct cx_gfx_buffer* p_buffer, size_t offset, size_t size, const void* p_data) {
	CX_ASSERT(offset + size <= p_buffer->size_, GFX_BUFFER);
	const struct cx_gfx_buffer_gl_internals* p_internals = (const void*)p_buffer->internals_.bytes_;
	glBindBuffer(p_internals->target, p_internals->id);
	glBufferSubData(p_internals->target, (GLintptr)offset, (GLsizeiptr)size, p_data);
}
