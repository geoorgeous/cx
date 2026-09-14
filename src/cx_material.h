#ifndef CX_MATERIAL_H
#define CX_MATERIAL_H

#include <stddef.h>
#include <stdint.h>

#include "cx_asset_defs.h"
#include "cx_gfx_shader_program_interface.h"
#include "cx_render_pipeline.h"

#define CX_LOG_CAT_MATERIAL "material"

#define CX_ASSET_TYPE_MATERIAL 3

#define CX_MATERIAL_MAX_PARAMS 16

struct cx_stream;

union cx_material_parameter_value {
	uint32_t u32;
	int32_t  i32;
	float    f32;
	uint32_t uvec2[2];
	uint32_t uvec3[3];
	uint32_t uvec4[4];
	int32_t  ivec2[2];
	int32_t  ivec3[3];
	int32_t  ivec4[4];
	float    fvec2[2];
	float    fvec3[3];
	float    fvec4[4];
	float    mat2[4];
	float    mat3[9];
	float    mat4[16];
	float    mat2x3[6];
	float    mat2x4[8];
	float    mat3x2[6];
	float    mat3x4[12];
	float    mat4x2[8];
	float    mat4x3[12];
};

struct cx_material_parameter_info {
	const char* s_name;
	enum cx_gfx_shader_program_value_type type;
};

struct cx_material_texture_info {
	const char* s_name;
};

struct cx_material_block_member_info {
	const char* s_name;
	enum cx_gfx_shader_program_value_type type;
	uint16_t array_len;
	size_t   array_stride;
	size_t matrix_stride;
};

struct cx_material_block_info {
	const char* s_name;
	struct cx_material_block_member_info* p_members;
	uint16_t num_members;
};

struct cx_material_property_info {
	struct cx_material_parameter_info* p_parameters;
	uint16_t num_parameters;

	struct cx_material_texture_info* p_textures;
	uint16_t num_textures;

	struct cx_material_block_info* p_blocks;
	uint16_t num_blocks;
};

struct cx_material_property_value_view {
	char   name[CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN];
	size_t value_buffer_offset;
	size_t value_size;
};

struct cx_material_parameter {
	struct cx_material_property_value_view value_view;
	enum cx_gfx_shader_program_value_type type;
};

struct cx_material_texture {
	struct cx_material_property_value_view value_view;
};

struct cx_material_block_member {
	char name[CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN];
	enum cx_gfx_shader_program_value_type type;
	size_t block_offset;
	size_t size;
	uint16_t array_len;
	size_t   array_stride;
	size_t matrix_stride;
};

struct cx_material_block {
	struct cx_material_property_value_view value_view;
	struct cx_material_block_member* p_members;
	uint16_t num_members;
};

struct cx_material {
	int                 b_is_override;
	uint8_t*            p_overrides;
	struct cx_asset_ref parent_asset_ref;

	struct cx_asset_ref pipeline_shader_asset_ref;
	struct cx_render_pipeline_state pipeline_state;

	struct cx_material_parameter* p_parameters;
	struct cx_gfx_shader_program_input_parameter* p_shader_program_input_parameters;
	uint16_t num_parameters;

	struct cx_material_texture* p_textures;
	struct cx_gfx_shader_program_input_texture* p_shader_program_input_textures;
	uint16_t num_textures;

	struct cx_material_block* p_blocks;
	struct cx_gfx_shader_program_input_block* p_shader_program_input_blocks;
	uint16_t num_blocks;

	struct cx_gfx_shader_program_input_set shader_program_input_set;

	size_t property_value_buffer_size;
	void* p_property_value_buffer;
};

void cx_material_create(
	const struct cx_asset_ref* p_pipeline_shader_asset_ref,
	const struct cx_render_pipeline_state* p_pipeline_state,
	const struct cx_material_property_info* p_property_info,
	struct cx_material* p_out);
void cx_material_create_override(const struct cx_asset_ref* p_parent, struct cx_material* p_out);
void cx_material_free(struct cx_material* p_material);
int cx_material_serialize(const struct cx_material* p_material, struct cx_stream* p_stream);
int cx_material_deserialize(struct cx_stream* p_stream, struct cx_material* p_material);
void cx_material_set_parameter(struct cx_material* p_material, const char* s_param_name, const void* p_value);
void cx_material_set_texture(
	struct cx_material* p_material, const char* s_texture_name, const struct cx_asset_ref* p_texture_asset_ref);
void cx_material_set_block(struct cx_material* p_material, const char* s_block_name, const void* p_value);
void cx_material_set_block_member(
	struct cx_material* p_material, const char* s_block_name, const char* s_member_name, const void* p_value);

static inline int cx_material_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return cx_material_serialize(p_asset, p_stream);
}

static inline int cx_material_asset_deserialize(struct cx_stream* p_stream, void* p_asset) {
	return cx_material_deserialize(p_stream, p_asset);
}

static inline void cx_material_asset_free(void* p_asset) {
	cx_material_free(p_asset);
}

void cx_material_asset_enumerate_dependencies(
	const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr);

#endif
