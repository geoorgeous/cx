#ifndef CX_GFX_MESH_H
#define CX_GFX_MESH_H

#include <stddef.h>
#include <stdint.h>

#include "cx_gfx_buffer.h"
#include "cx_macro.h"

#define CX_GFX_MESH_MAX_ATTR_BUFFERS 8

struct cx_mesh_data;

struct cx_gfx_mesh {
	size_t num_elements_;
	float  aabb_min_[3];
	float  aabb_max_[3];
	uint64_t layout_hash_;
	CX_OPAQUE_INTERNALS(512);
};

void cx_gfx_mesh_create(
	const struct cx_mesh_data* p_mesh_data,
	enum cx_gfx_buffer_usage usage,
	struct cx_gfx_mesh* p_out);

void cx_gfx_mesh_destroy(struct cx_gfx_mesh* p_mesh);

void cx_gfx_mesh_update(struct cx_gfx_mesh* p_mesh, const struct cx_mesh_data* p_mesh_data);

void cx_gfx_mesh_draw(const struct cx_gfx_mesh* p_mesh);

#endif
