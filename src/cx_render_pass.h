#ifndef CX_RENDER_PASS_H
#define CX_RENDER_PASS_H

#include <stddef.h>

#include "cx_dbg.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"

#define CX_RENDER_PASS_MAX_OPAQUES 8

struct cx_render_pass_build_info {
	const struct cx_gfx_program_source program_source;
	const char* s_pass_block_name;
	const char* s_object_block_name;
	const char* s_material_block_name;
	const char** p_s_opaque_param_names;
	size_t num_opaque_params;
};

struct cx_render_pass {
	struct cx_gfx_program program;
	struct cx_gfx_program_param_block pass_block;
	struct cx_gfx_program_param_buffer pass_buffer;
	struct cx_gfx_program_param_block object_block;
	struct cx_gfx_program_param_buffer object_buffer;
	struct cx_gfx_program_param_block material_block;
	struct cx_gfx_program_param_buffer material_buffer;
	struct cx_gfx_program_opaque_param opaque_params[CX_RENDER_PASS_MAX_OPAQUES];
	size_t num_opaque_params;
};

struct cx_render_pass_execute_info {
	const struct cx_gfx_framebuffer* p_framebuffer;
	int32_t viewport[4];
	int b_clear_color;
	int b_clear_depth;
	int b_clear_stencil;
	float clear_color[4];
	float clear_depth;
	int clear_stencil;
};

struct cx_render_pass_data {
	void* p_data;
};

struct cx_render_command {
	const struct cx_gfx_mesh* p_mesh;
	const void* p_object_data;
	const void* p_material_data;
	const void* p_opaque_resources[CX_RENDER_PASS_MAX_OPAQUES];
	size_t num_opaque_params;
};

struct cx_render_command_buffer {
	struct cx_render_command* p_commands;
	size_t num;
	size_t capacity;
};

int cx_render_pass_build(const struct cx_render_pass_build_info* p_info, struct cx_render_pass* p_out);

void cx_render_pass_execute(
	const struct cx_render_pass* p_pass,
	const struct cx_render_pass_execute_info* p_info,
	const struct cx_render_pass_data* p_data,
	const struct cx_render_command_buffer* p_command_buffer);

static inline void cx_render_command_buffer_push(
	struct cx_render_command_buffer* p_buffer,
	const struct cx_render_command* p_command) {
	CX_ASSERT(p_buffer->num < p_buffer->capacity);
	p_buffer->p_commands[p_buffer->num++] = *p_command;
}

#endif
