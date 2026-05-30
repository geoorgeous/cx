#ifndef CX_OBJECT_ID_CAPTURER_H
#define CX_OBJECT_ID_CAPTURER_H

#include <stdint.h>

#include "cx_gfx_framebuffer.h"
#include "cx_render_pass.h"
#include "cx_gfx_texture.h"

#define CX_LOG_CAT_OBJECT_ID_CAPTURER "object_id_capturer"

#define CX_OBJECT_ID_CAPTURER_MAX_OBJECTS 1024

#define CX_OBJECT_ID_NONE 0

#define CX_OBJECT_ID_CATEGORY_BITS  8u
#define CX_OBJECT_ID_PAYLOAD_BITS  24u

#define CX_OBJECT_ID_MAKE(CATEGORY, PAYLOAD)\
	(((((uint32_t)(CATEGORY)) & 0xffu) << (CX_OBJECT_ID_PAYLOAD_BITS) |\
	(((uint32_t)(PAYLOAD)) & 0xffffffu)))

#define CX_OBJECT_ID_GET_CATEGORY(ID)\
	(((uint32_t)(ID)) >> (CX_OBJECT_ID_PAYLOAD_BITS))

#define CX_OBJECT_ID_GET_PAYLOAD(ID)\
	(((uint32_t)(ID)) & ((1u << (CX_OBJECT_ID_PAYLOAD_BITS)) - 1u))
	
struct cx_gfx_mesh;

struct cx_object_id_capturer_object_data {
	float transform[16];
	uint32_t id;
};

struct cx_object_id_capturer {
    uint32_t                  framebuffer_width;
	uint32_t                  framebuffer_height;
	struct cx_gfx_framebuffer framebuffer;
	struct cx_gfx_texture     framebuffer_color;
	struct cx_gfx_texture     framebuffer_depth_stencil;
};

struct cx_object_id_capturer_item {
	const struct cx_gfx_mesh* p_mesh;
	const float*              p_transform;
	uint32_t                  id;
};

void cx_object_id_capturer_free(struct cx_object_id_capturer* p_capturer);

void cx_object_id_capturer_draw(
	struct cx_object_id_capturer* p_capturer,
	const float* p_projection_matrix,
	const float* p_view_matrix,
	uint32_t fb_width,
	uint32_t fb_height,
	const struct cx_render_command_buffer* p_render_command_buffer);

uint32_t cx_object_id_capturer_query(const struct cx_object_id_capturer* p_capturer, float x, float y);

#endif
