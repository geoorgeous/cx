#ifndef CX_GFX_BUFFER_H
#define CX_GFX_BUFFER_H

#include <stddef.h>

#include "cx_macro.h"

#define CX_LOG_CAT_GFX_BUFFER "gfx:buffer"

enum cx_gfx_buffer_type {
	CX_GFX_BUFFER_TYPE_vertex,
	CX_GFX_BUFFER_TYPE_index,
	CX_GFX_BUFFER_TYPE_uniform
};

enum cx_gfx_buffer_usage {
	CX_GFX_BUFFER_USAGE_static,
	CX_GFX_BUFFER_USAGE_dynamic
};

struct cx_gfx_buffer {
	enum cx_gfx_buffer_type type_;
	enum cx_gfx_buffer_usage usage_;
	size_t size_;
	CX_OPAQUE_INTERNALS(20);
};

void cx_gfx_buffer_create(
	enum cx_gfx_buffer_type type,
	size_t size,
	enum cx_gfx_buffer_usage usage,
	struct cx_gfx_buffer* p_out);

void cx_gfx_buffer_destroy(struct cx_gfx_buffer* p_buffer);

void cx_gfx_buffer_set(struct cx_gfx_buffer* p_buffer, size_t size, const void* p_data);

void cx_gfx_buffer_set_region(const struct cx_gfx_buffer* p_buffer, size_t offset, size_t size, const void* p_data);

#endif
