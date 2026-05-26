#ifndef CX_RENDER_PASS_H
#define CX_RENDER_PASS_H

#include <stddef.h>

#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"

struct cx_render_pass {
	const struct cx_gfx_program* p_program;
	const struct cx_gfx_program_param_block* p_pass_block;
	const struct cx_gfx_program_param_buffer* p_pass_buffer;
	const struct cx_gfx_program_param_block* p_object_block;
	const struct cx_gfx_program_param_buffer* p_object_buffer;
	const struct cx_gfx_program_param_block* p_material_block;
	const struct cx_gfx_program_param_buffer* p_material_buffer;
};

struct cx_render_pass_data {
	void* p_data;
};

struct cx_render_pass_command {
	const struct cx_gfx_mesh* p_mesh;
	const void* p_object_data;
	const void* p_material_data;
	const struct cx_gfx_program_opaque_param_binding* p_opaque_params;
	size_t num_opaque_params;
};

void cx_render_pass_execute(
	const struct cx_render_pass* p_pass,
	const struct cx_render_pass_data* p_data,
	const struct cx_render_pass_command* p_commands,
	size_t n);

#endif
