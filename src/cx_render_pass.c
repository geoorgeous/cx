#include "cx_render_pass.h"

void cx_render_pass_execute(
	const struct cx_render_pass* p_pass,
	const struct cx_render_pass_data* p_data,
	const struct cx_render_pass_command* p_commands,
	size_t n) {

	cx_gfx_program_bind(p_pass->p_program);

	cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
		.p_block = p_pass->p_pass_block,
		.p_buffer = p_pass->p_pass_buffer
	}));

	cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
		.p_block = p_pass->p_object_block,
		.p_buffer = p_pass->p_object_buffer
	}));

	if (p_pass->p_material_buffer) {
		cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
			.p_block = p_pass->p_material_block,
			.p_buffer = p_pass->p_material_buffer
		}));
	}
	
	cx_gfx_program_param_buffer_set(p_pass->p_pass_buffer, 0, 0, p_data->p_data);
	
	for (size_t i = 0; i < n; ++i) {
		const struct cx_render_pass_command* p_command = p_commands + i;

		cx_gfx_program_param_buffer_set(p_pass->p_object_buffer, 0, 0, p_command->p_object_data);
		
		if (p_pass->p_material_buffer) {
			cx_gfx_program_param_buffer_set(p_pass->p_material_buffer, 0, 0, p_command->p_material_data);		
		}

		for (size_t j = 0; j < p_command->num_opaque_params; ++j) {
			const struct cx_gfx_program_opaque_param_binding* p_binding = p_command->p_opaque_params + j;
			cx_gfx_program_opaque_param_bind_resource(p_binding);
		}

		cx_gfx_mesh_draw(p_command->p_mesh);
	}
}
