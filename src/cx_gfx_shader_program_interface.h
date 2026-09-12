#ifndef CX_GFX_SHADER_PROGRAM_INTERFACE_H
#define CX_GFX_SHADER_PROGRAM_INTERFACE_H

#include <stddef.h>
#include <stdint.h>

#include "cx_macro.h"

#define CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN 64

enum cx_gfx_shader_program_value_type {
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_bool,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_u32,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_i32,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_f32,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_uvec2,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_ivec2,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_fvec2,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_uvec3,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_ivec3,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_fvec3,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_uvec4,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_ivec4,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_fvec4,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat2,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat3,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat4,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat2x3,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat2x4,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat3x2,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat3x4,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat4x2,
	CX_GFX_SHADER_PROGRAM_VALUE_TYPE_mat4x3
};

enum cx_gfx_shader_program_texture_type {
	CX_GFX_SHADER_PROGRAM_TEXTURE_TYPE_texture_2d,
	CX_GFX_SHADER_PROGRAM_TEXTURE_TYPE_texture_cube,
};

struct cx_gfx_shader_program_parameter_info {
	char name[CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN];
	enum cx_gfx_shader_program_value_type type;
	CX_OPAQUE_INTERNALS(4);
};

struct cx_gfx_shader_program_texture_info {
	char name[CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN];
	enum cx_gfx_shader_program_texture_type type;
	CX_OPAQUE_INTERNALS(8);
};

struct cx_gfx_shader_program_block_member_info {
	char name[CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN];
	enum cx_gfx_shader_program_value_type type;
	size_t offset;
	uint16_t array_len;
	size_t array_stride;
	size_t matrix_stride;
};

struct cx_gfx_shader_program_block_info {
	char name[CX_GFX_SHADER_PROGRAM_NAME_MAX_LEN];
	size_t size;
	struct cx_gfx_shader_program_block_member_info* p_members;
	uint16_t num_members;
	CX_OPAQUE_INTERNALS(4);
};

struct cx_gfx_shader_program_interface {
	struct cx_gfx_shader_program_parameter_info* p_parameters;
	uint16_t num_parameters;
	struct cx_gfx_shader_program_texture_info* p_textures;
	uint16_t num_textures;
	struct cx_gfx_shader_program_block_info* p_blocks;
	uint16_t num_blocks;
};

struct cx_gfx_shader_program_input_parameter {
	const char* s_name;
	enum cx_gfx_shader_program_value_type type;
	const void* p_value;
};

struct cx_gfx_shader_program_input_texture {
	const char* s_name;
	const struct cx_gfx_texture* p_texture;
};

struct cx_gfx_shader_program_input_block {
	const char* s_name;
	const void* p_data;
};

struct cx_gfx_shader_program_input_set {
	const struct cx_gfx_shader_program_input_parameter* p_parameters;
	uint16_t num_parameters;
	const struct cx_gfx_shader_program_input_texture* p_textures;
	uint16_t num_textures;
	const struct cx_gfx_shader_program_input_block* p_blocks;
	uint16_t num_blocks;
};

#endif
