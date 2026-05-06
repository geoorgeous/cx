#ifndef MESH_ID_CAPTURER_H
#define MESH_ID_CAPTURER_H

#include <stdint.h>

#include "cx_gfx_framebuffer.h"
#include "cx_gfx_texture.h"

struct cx_gfx_mesh;

struct mesh_id_capturer {
    uint32_t                  framebuffer_size[2];
	struct cx_gfx_framebuffer framebuffer;
	struct cx_gfx_texture     framebuffer_color;
	struct cx_gfx_texture     framebuffer_depth_stencil;
};

struct mesh_id_capturer_item {
	const struct cx_gfx_mesh* p_mesh;
	const float*              p_transform;
	unsigned int              id;
};

void mesh_id_capturer_free(struct mesh_id_capturer* p_mesh_id_capturer);

void mesh_id_capturer_begin(
	struct mesh_id_capturer* p_mesh_id_capturer,
	const uint32_t* p_framebuffer_size,
	const float* p_projection_matrix,
	const float* p_view_matrix);

void mesh_id_capturer_draw_item(const struct mesh_id_capturer_item* p_item);

unsigned int mesh_id_capturer_query(
	const struct mesh_id_capturer* p_mesh_id_capturer,
	const float* p_normalized_coordinates);

#endif
