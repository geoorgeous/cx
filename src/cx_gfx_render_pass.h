#ifndef CX_GFX_RENDER_PASS_H
#define CX_GFX_RENDER_PASS_H

#include <stdint.h>

#include "cx_render_draw_command.h"
#include "cx_gfx_shader_program_interface.h"

#define CX_LOG_CAT_GFX_RENDER_PASS "render_pass"

enum cx_gfx_render_target_clear_flag {
	CX_GFX_RENDER_TARGET_CLEAR_FLAG_none    = 0x0,
	CX_GFX_RENDER_TARGET_CLEAR_FLAG_color   = 0x1,
	CX_GFX_RENDER_TARGET_CLEAR_FLAG_depth   = 0x2,
	CX_GFX_RENDER_TARGET_CLEAR_FLAG_stencil = 0x4,
};

struct cx_gfx_framebuffer;

struct cx_gfx_render_pass {
	const struct cx_gfx_framebuffer* p_framebuffer;
	int32_t viewport[4];
	int8_t  clear_flags;
	float   clear_color[4];
	float   clear_depth;
	int32_t clear_stencil;
	struct cx_gfx_shader_program_input_set pass_input_set;
};

void cx_gfx_render_pass_execute(
	const struct cx_gfx_render_pass* p_render_pass,
	struct cx_render_draw_command* p_draw_commands,
	uint32_t num_draw_commands);

#endif
