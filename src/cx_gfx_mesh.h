#ifndef _H__CX_GFX_MESH
#define _H__CX_GFX_MESH

#include <stddef.h>

#define CX_GFX_MESH_MAX_ATTR_BUFFERS 8

struct mesh_primitive;

struct cx_gfx_mesh {
	size_t _elements_count;
	float  _bounds_min[3];
	float  _bounds_max[3];
	char   _bytes[20 + (4 * CX_GFX_MESH_MAX_ATTR_BUFFERS)];
};

void cx_gfx_mesh_create(struct cx_gfx_mesh* p_mesh, const struct mesh_primitive* p_mesh_primitive);
void cx_gfx_mesh_destroy(struct cx_gfx_mesh* p_mesh);
void cx_gfx_mesh_draw(const struct cx_gfx_mesh* p_mesh);

#endif
