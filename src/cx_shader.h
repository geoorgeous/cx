#ifndef CX_SHADER_H
#define CX_SHADER_H

#include <stddef.h>
#include <stdint.h>

#include "cx_macro.h"

#define CX_ASSET_TYPE_SHADER 20

enum cx_shader_param_type {
	CX_SHADER_PARAM_TYPE_u32,
	CX_SHADER_PARAM_TYPE_i32,
	CX_SHADER_PARAM_TYPE_f32,
	CX_SHADER_PARAM_TYPE_uvec2,
	CX_SHADER_PARAM_TYPE_ivec2,
	CX_SHADER_PARAM_TYPE_fvec2,
	CX_SHADER_PARAM_TYPE_uvec3,
	CX_SHADER_PARAM_TYPE_ivec3,
	CX_SHADER_PARAM_TYPE_fvec3,
	CX_SHADER_PARAM_TYPE_uvec4,
	CX_SHADER_PARAM_TYPE_ivec4,
	CX_SHADER_PARAM_TYPE_fvec4,
	CX_SHADER_PARAM_TYPE_mat2,
	CX_SHADER_PARAM_TYPE_mat3,
	CX_SHADER_PARAM_TYPE_mat4,
	CX_SHADER_PARAM_TYPE_mat2x3,
	CX_SHADER_PARAM_TYPE_mat2x4,
	CX_SHADER_PARAM_TYPE_mat3x2,
	CX_SHADER_PARAM_TYPE_mat3x4,
	CX_SHADER_PARAM_TYPE_mat4x2,
	CX_SHADER_PARAM_TYPE_mat4x3,
};

struct cx_shader_binding_param {
	const char* s_param_name;
	enum cx_shader_param_type type;
	CX_OPAQUE_INTERNALS(4);
};

struct cx_shader_binding_block {
	const char* s_name;
	size_t size;
	CX_OPAQUE_INTERNALS(4);
};

struct cx_shader_binding_block_member {
	const char* s_name;
	enum cx_shader_param_type type;
	size_t offset;
	size_t size;
	CX_OPAQUE_INTERNALS(4);
};

enum cx_shader_sampler_type {
	CX_SHADER_SAMPLER_sampler2d,
	CX_SHADER_SAMPLER_cubemap
};

struct cx_shader_binding_sampler {
	const char* s_name;
	enum cx_shader_sampler_type type;
	CX_OPAQUE_INTERNALS(8);
};

enum cx_shader_binding_type {
	CX_SHADER_BINDING_TYPE_param,
	CX_SHADER_BINDING_TYPE_block,
	CX_SHADER_BINDING_TYPE_block_member,
	CX_SHADER_BINDING_TYPE_sampler,
};

struct cx_shader_binding {
	enum cx_shader_binding_type type;
	union {
		struct cx_shader_binding_param param;
		struct cx_shader_binding_block block;
		struct cx_shader_binding_block_member block_member;
		struct cx_shader_binding_sampler sampler;
	} data;
};

struct cx_shader_source {
	const char* p_vertex_source;
	uint32_t vertex_source_len;
	const char* p_fragment_source;
	uint32_t fragment_source_len;
};

struct cx_shader {
	struct cx_shader_source source;
	struct cx_shader_binding_block* p_block_bindings;
	uint16_t num_block_bindings;
};

void cx_shader_create(struct cx_shader* p_out);
void cx_shader_destroy(struct cx_shader* p_shader);
void cx_shader_build(struct cx_shader* p_shader, const struct cx_shader_source* p_source);
void cx_shader_bind(const struct cx_shader* p_shader);
int cx_shader_find_binding(const struct cx_shader* p_shader, const char* s_id, const struct cx_shader_binding** pp_out);

struct cx_stream;

int cx_shader_asset_serialize(const void* p_asset, struct cx_stream* p_stream);
int cx_shader_asset_deserialize(struct cx_stream* p_stream, void* p_asset);

static inline void cx_shader_asset_free(void* p_asset) {
	struct cx_shader* p_shader = p_asset;
	cx_shader_destroy(p_shader);
}

#endif
