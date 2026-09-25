#ifndef CX_RENDER_DRAW_COMMAND_H
#define CX_RENDER_DRAW_COMMAND_H

#include "cx_gfx_shader_program_interface.h"
#include "cx_render_pipeline.h"

struct cx_gfx_mesh;

struct cx_render_draw_command {
	struct cx_render_pipeline pipeline;
	struct cx_gfx_shader_program_input_set material_input_set;
	struct cx_gfx_shader_program_input_set draw_input_set;
	const struct cx_gfx_mesh* p_mesh;
	int b_scissor;
	int16_t scissor_x;
	int16_t scissor_y;
	uint16_t scissor_width;
	uint16_t scissor_height;
};

struct cx_render_draw_command_buffer {
	struct cx_render_draw_command* p_first;
	uint32_t capacity;
	uint32_t len;
};

static inline void cx_render_draw_command_buffer_push(
	struct cx_render_draw_command_buffer* p_buffer, const struct cx_render_draw_command* p_command) {
	
	p_buffer->p_first[p_buffer->len] = *p_command;
	p_buffer->len++;
}

#endif
