#ifndef CX_RENDER_PARAMETER_H
#define CX_RENDER_PARAMETER_H

#include <stdint.h>

#include "cx_asset_defs.h"

enum cx_render_param_type {
	CX_RENDER_PARAM_TYPE_u32,
	CX_RENDER_PARAM_TYPE_i32,
	CX_RENDER_PARAM_TYPE_f32,
	CX_RENDER_PARAM_TYPE_uvec2,
	CX_RENDER_PARAM_TYPE_ivec2,
	CX_RENDER_PARAM_TYPE_fvec2,
	CX_RENDER_PARAM_TYPE_uvec3,
	CX_RENDER_PARAM_TYPE_ivec3,
	CX_RENDER_PARAM_TYPE_fvec3,
	CX_RENDER_PARAM_TYPE_uvec4,
	CX_RENDER_PARAM_TYPE_ivec4,
	CX_RENDER_PARAM_TYPE_fvec4,
	CX_RENDER_PARAM_TYPE_mat2,
	CX_RENDER_PARAM_TYPE_mat3,
	CX_RENDER_PARAM_TYPE_mat4,
	CX_RENDER_PARAM_TYPE_mat2x3,
	CX_RENDER_PARAM_TYPE_mat2x4,
	CX_RENDER_PARAM_TYPE_mat3x2,
	CX_RENDER_PARAM_TYPE_mat3x4,
	CX_RENDER_PARAM_TYPE_mat4x2,
	CX_RENDER_PARAM_TYPE_mat4x3,
	CX_RENDER_PARAM_TYPE_block,
	CX_RENDER_PARAM_TYPE_texture
};

struct cx_render_param {
	const char* s_name;
	enum cx_render_param_type type;
	void* p_data;
};

struct cx_render_param_set {
	struct cx_render_param* p_params;
	uint16_t num_params;
};

struct cx_stream;

int cx_render_param_set_serialize(const struct cx_render_param_set* p_render_param_set, struct cx_stream* p_stream);
int cx_render_param_set_deserialize(struct cx_stream* p_stream, struct cx_render_param_set* p_render_param_set);
void cx_render_param_set_enumerate_dependencies(
	const struct cx_render_param_set* p_render_param_set,
	cx_asset_enumerate_dependencies_cb_fn f_cb,
	void* p_user_ptr);

#endif
