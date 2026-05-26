#include "cx_gfx_framebuffer.h"
#include "cx_render_pass.h"
#include "gl.h"

int cx_render_pass_build(const struct cx_render_pass_build_info* p_info, struct cx_render_pass* p_out) {
	CX_ASSERT(p_info->num_opaque_params <= CX_RENDER_PASS_MAX_OPAQUES);

	*p_out = (struct cx_render_pass){0};
	
	if (cx_gfx_program_create(&p_out->program) != CX_ERROR_none) {
		return 0;
	}

	if (cx_gfx_program_build(&p_out->program, &p_info->program_source) != CX_ERROR_none) {
		return 0;
	}

	if (p_info->s_pass_block_name && (
		!cx_gfx_program_refl_param_block(
			&p_out->program,
			p_info->s_pass_block_name,
			&p_out->pass_block) ||
		cx_gfx_program_param_buffer_create(
			&p_out->pass_buffer,
			p_out->pass_block.size_) != CX_ERROR_none)) {
		return 0;
	}

	if (p_info->s_object_block_name && (
		!cx_gfx_program_refl_param_block(
			&p_out->program,
			p_info->s_object_block_name,
			&p_out->object_block) ||
		cx_gfx_program_param_buffer_create(
			&p_out->object_buffer,
			p_out->object_block.size_) != CX_ERROR_none)) {
		return 0;
	}
	
	if (p_info->s_material_block_name && (
		!cx_gfx_program_refl_param_block(
			&p_out->program,
			p_info->s_material_block_name,
			&p_out->material_block) ||
		cx_gfx_program_param_buffer_create(
			&p_out->material_buffer,
			p_out->material_block.size_) != CX_ERROR_none)) {
		return 0;
	}

	for (size_t i = 0; i < p_info->num_opaque_params; ++i) {
		if (!cx_gfx_program_refl_opaque_param(
				&p_out->program,
				p_info->p_s_opaque_param_names[i],
				&p_out->opaque_params[i])) {
			return 0;
		}
	}

	p_out->num_opaque_params = p_info->num_opaque_params;

	return 1;
}

void cx_render_pass_execute(
	const struct cx_render_pass* p_pass,
	const struct cx_render_pass_execute_info* p_info,
	const struct cx_render_pass_data* p_data,
	const struct cx_render_command_buffer* p_command_buffer) {

	if (p_info->p_framebuffer) {
		cx_gfx_framebuffer_bind(p_info->p_framebuffer);
	}

	glViewport(
		p_info->viewport[0],
		p_info->viewport[1],
		p_info->viewport[2],
		p_info->viewport[3]);

	if (p_info->b_clear_color) {
		glClearColor(
			p_info->clear_color[0],
			p_info->clear_color[1],
			p_info->clear_color[2],
			p_info->clear_color[3]);
	}

	if (p_info->b_clear_depth) {
		glClearDepth(p_info->clear_depth);
	}

	if (p_info->b_clear_stencil) {
		glClearStencil(p_info->clear_stencil);
	}
	
	const GLbitfield clear_mask =
		(GL_COLOR_BUFFER_BIT * !!p_info->b_clear_color) |
		(GL_DEPTH_BUFFER_BIT * !!p_info->b_clear_depth) |
		(GL_STENCIL_BUFFER_BIT * !!p_info->b_clear_stencil);

	if (clear_mask) {
		glClear(clear_mask);
	}

	cx_gfx_program_bind(&p_pass->program);

	cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
		.p_block = &p_pass->pass_block,
		.p_buffer = &p_pass->pass_buffer
	}));

	cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
		.p_block = &p_pass->object_block,
		.p_buffer = &p_pass->object_buffer
	}));

	if (p_pass->material_buffer.size) {
		cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
			.p_block = &p_pass->material_block,
			.p_buffer = &p_pass->material_buffer
		}));
	}
	
	cx_gfx_program_param_buffer_set(&p_pass->pass_buffer, 0, 0, p_data->p_data);
	
	for (size_t i = 0; i < p_command_buffer->num; ++i) {
		const struct cx_render_command* p_command = &p_command_buffer->p_commands[i];

		cx_gfx_program_param_buffer_set(&p_pass->object_buffer, 0, 0, p_command->p_object_data);
		
		if (p_pass->material_buffer.size) {
			cx_gfx_program_param_buffer_set(&p_pass->material_buffer, 0, 0, p_command->p_material_data);		
		}

		for (size_t j = 0; j < p_command->num_opaque_params; ++j) {
			cx_gfx_program_opaque_param_bind_resource(&((struct cx_gfx_program_opaque_param_binding) {
				.p_param = &p_pass->opaque_params[j],
				p_command->p_opaque_resources[j]
			}));
		}

		cx_gfx_mesh_draw(p_command->p_mesh);
	}
}
