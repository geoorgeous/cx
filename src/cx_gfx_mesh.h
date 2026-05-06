#ifndef CX_GFX_MESH_H
#define CX_GFX_MESH_H

#include <stddef.h>

#define CX_GFX_MESH_MAX_ATTR_BUFFERS 8

struct mesh_primitive;

struct cx_gfx_mesh {
	size_t elements_count_;
	float  bounds_min_[3];
	float  bounds_max_[3];
	char   bytes_[20 + (4 * CX_GFX_MESH_MAX_ATTR_BUFFERS)];
};

void cx_gfx_mesh_create(struct cx_gfx_mesh* p_mesh, const struct mesh_primitive* p_mesh_primitive);
void cx_gfx_mesh_destroy(struct cx_gfx_mesh* p_mesh);
void cx_gfx_mesh_draw(const struct cx_gfx_mesh* p_mesh);

#endif
