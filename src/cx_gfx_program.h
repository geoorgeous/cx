#ifndef CX_GFX_PROGRAM_H
#define CX_GFX_PROGRAM_H

#include <stddef.h>

#include "errors.h"

#define CX_LOG_CAT_GFX_PROGRAM "gfx:program"

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
	CX_GFX_PROGRAM_PARAM_TYPE_mat4x3
};

enum cx_gfx_program_opaque_param_type {
	CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_unknown,
	CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_sampler_2d,
	CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_sampler_cube
};

struct cx_gfx_program_param_buffer {
	size_t size;
	char   _bytes[4];
};

enum error cx_gfx_program_param_buffer_create(struct cx_gfx_program_param_buffer* p_buffer, size_t size);

void       cx_gfx_program_param_buffer_destroy(struct cx_gfx_program_param_buffer* p_buffer);

void       cx_gfx_program_param_buffer_bind(const struct cx_gfx_program_param_buffer* p_buffer, unsigned int index);

void       cx_gfx_program_param_buffer_bind_range(
	const struct cx_gfx_program_param_buffer* p_buffer,
	unsigned int index,
	size_t offset,
	size_t size);

void       cx_gfx_program_param_buffer_set(
	const struct cx_gfx_program_param_buffer* p_buffer,
	size_t offset,
	size_t size,
	const void* p_data);

struct cx_gfx_program_opaque_param {
	enum cx_gfx_program_opaque_param_type type;
	size_t                                n;
	unsigned int                          slot;
	char _bytes[4];
};

void cx_gfx_program_opaque_param_bind_resource(
	const struct cx_gfx_program_opaque_param* p_opaque_param,
	const void* p_resource);

struct cx_gfx_program_param_block {
	size_t       _size;
	char         _bytes[4];
};

void cx_gfx_program_param_block_bind_buffer(
	const struct cx_gfx_program_param_block* p_param_block,
	const struct cx_gfx_program_param_buffer* p_buffer,
	size_t buffer_data_offset,
	size_t buffer_data_size);

struct cx_gfx_program_source {
	const char* s_vertex_stage_source;
	const char* s_fragment_stage_source;
};

struct cx_gfx_program {
	char _bytes[4];
};

enum error cx_gfx_program_create(struct cx_gfx_program* p_program);

void       cx_gfx_program_destroy(struct cx_gfx_program* p_program);

int        cx_gfx_program_is_built(struct cx_gfx_program* p_program);

enum error cx_gfx_program_build(struct cx_gfx_program* p_program, const struct cx_gfx_program_source* p_source);

int        cx_gfx_program_refl_opaque_param(
	const struct cx_gfx_program* p_program,
	const char* s_name,
	struct cx_gfx_program_opaque_param* p_out_opque_param);

int        cx_gfx_program_refl_param_block(
	const struct cx_gfx_program* p_program,
	const char* s_name,
	struct cx_gfx_program_param_block* p_out_param_block);

void       cx_gfx_program_bind(const struct cx_gfx_program* p_program);

#endif
