#ifndef CX_GFX_PROGRAM_H
#define CX_GFX_PROGRAM_H

#include <stddef.h>
#include <stdint.h>

#include "cx_macro.h"
#include "cx_result.h"

#define CX_LOG_CAT_GFX_PROGRAM "gfx:program"

#define CX_GFX_PROGRAM_MAX_PARAM_BLOCKS 4
#define CX_GFX_PROGRAM_MAX_PARAMS 8

enum cx_gfx_program_param_type {
	CX_GFX_PROGRAM_PARAM_TYPE_unknown,
	CX_GFX_PROGRAM_PARAM_TYPE_u32,
	CX_GFX_PROGRAM_PARAM_TYPE_i32,
	CX_GFX_PROGRAM_PARAM_TYPE_f32,
	CX_GFX_PROGRAM_PARAM_TYPE_uvec2,
	CX_GFX_PROGRAM_PARAM_TYPE_ivec2,
	CX_GFX_PROGRAM_PARAM_TYPE_fvec2,
	CX_GFX_PROGRAM_PARAM_TYPE_uvec3,
	CX_GFX_PROGRAM_PARAM_TYPE_ivec3,
	CX_GFX_PROGRAM_PARAM_TYPE_fvec3,
	CX_GFX_PROGRAM_PARAM_TYPE_uvec4,
	CX_GFX_PROGRAM_PARAM_TYPE_ivec4,
	CX_GFX_PROGRAM_PARAM_TYPE_fvec4,
	CX_GFX_PROGRAM_PARAM_TYPE_mat2,
	CX_GFX_PROGRAM_PARAM_TYPE_mat3,
	CX_GFX_PROGRAM_PARAM_TYPE_mat4,
	CX_GFX_PROGRAM_PARAM_TYPE_mat2x3,
	CX_GFX_PROGRAM_PARAM_TYPE_mat2x4,
	CX_GFX_PROGRAM_PARAM_TYPE_mat3x2,
	CX_GFX_PROGRAM_PARAM_TYPE_mat3x4,
	CX_GFX_PROGRAM_PARAM_TYPE_mat4x2,
	CX_GFX_PROGRAM_PARAM_TYPE_mat4x3,
};

enum cx_gfx_program_opaque_type {
	CX_GFX_PROGRAM_OPQUE_TYPE_sampler2d,
	CX_GFX_PROGRAM_OPQUE_TYPE_cubemap,
};

struct cx_gfx_program_param {
	enum cx_gfx_program_param_type type;
	union {
		CX_OPAQUE_INTERNALS(4);
		uint16_t block_index;
	} u;
};

struct cx_gfx_program_param_block {
	size_t size;
	CX_OPAQUE_INTERNALS(4);
};

struct cx_gfx_program_source {
	const char* s_vertex_stage_source;
	const char* s_fragment_stage_source;
};

struct cx_gfx_program {
	struct cx_gfx_program_param params[CX_GFX_PROGRAM_MAX_PARAMS];
	uint16_t num_params;
	struct cx_gfx_program_param_block param_blocks[CX_GFX_PROGRAM_MAX_PARAM_BLOCKS];
	uint16_t num_param_blocks;
	CX_OPAQUE_INTERNALS(4);
};







struct cx_gfx_program_binding_param {
	CX_OPAQUE_INTERNALS(4); // opengl uniform loc
};

struct cx_gfx_program_binding_block {
	size_t size;
	CX_OPAQUE_INTERNALS(4); // opengl binding point
};

struct cx_gfx_program_binding_block_member {
	size_t offset;
	size_t size;
	CX_OPAQUE_INTERNALS(4);
};

struct cx_gfx_program_binding_sampler {
	CX_OPAQUE_INTERNALS(4); // opengl texture unit
};

enum cx_gfx_program_resource_type {
	CX_GFX_PROGRAM_RESOURCE_TYPE_param,
	CX_GFX_PROGRAM_RESOURCE_TYPE_param_block,
	CX_GFX_PROGRAM_RESOURCE_TYPE_opaque,
};

struct cx_gfx_program_resource_binding {
	const char* s_name;
	enum cx_gfx_program_resource_type type;
	union {
		struct cx_gfx_program_binding_param as_param;
		struct cx_gfx_program_binding_block as_param_block;
		struct cx_gfx_program_sampler ;
	} info;
};

struct cx_gfx_program_interface {
};

cx_result cx_gfx_program_create(struct cx_gfx_program* p_program);

void      cx_gfx_program_destroy(struct cx_gfx_program* p_program);

int       cx_gfx_program_is_built(struct cx_gfx_program* p_program);

cx_result cx_gfx_program_build(struct cx_gfx_program* p_program, const struct cx_gfx_program_source* p_source);

void      cx_gfx_program_bind(const struct cx_gfx_program* p_program);

void cx_gfx_program_bind_resource(
	const struct cx_gfx_program* p_program,
	const struct cx_gfx_program_resource_binding* p_binding, 
	const void* p_resource);

int cx_gfx_program_interface_find_resource_binding(
	const struct cx_gfx_program_interface* p_interface,
	const char* s_name,
	const struct cx_gfx_program_resource_binding** pp_out);

#endif
