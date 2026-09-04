#ifndef CX_RENDER_PASS_H
#define CX_RENDER_PASS_H

#include <stdint.h>

#include "cx_render_draw_command.h"
#include "cx_render_parameter.h"

#define CX_LOG_CAT_RENDER_PASS "render_pass"

enum cx_gfx_render_target_clear_mask_bit {
	CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_none    = 0x0,
	CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_color  = 0x1,
	CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_depth   = 0x2,
	CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_stencil = 0x4,
};

struct cx_gfx_framebuffer;

struct cx_render_pass {
	const struct cx_gfx_framebuffer* p_framebuffer;
	int32_t viewport[4];
	int8_t  clear_mask;
	float   clear_color[4];
	float   clear_depth;
	int32_t clear_stencil;
	struct cx_render_param_set param_set;
};

void cx_render_pass_execute(
	const struct cx_render_pass* p_render_pass,
	struct cx_render_draw_command* p_draw_commands,
	uint32_t num_draw_commands);

#endif
