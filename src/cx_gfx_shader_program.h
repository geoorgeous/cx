#ifndef CX_GFX_SHADER_PROGRAM_H
#define CX_GFX_SHADER_PROGRAM_H

#include "cx_macro.h"
#include "cx_result.h"

#define CX_LOG_CAT_GFX_SHADER_PROGRAM "gfx:shader_program"

struct cx_gfx_shader_program {
	CX_OPAQUE_INTERNALS(4);
};

struct cx_gfx_shader_program_interface;
struct cx_shader_source;

cx_result cx_gfx_shader_program_create(struct cx_gfx_shader_program* p_out);
void cx_gfx_shader_program_destroy(struct cx_gfx_shader_program* p_shader_program);
cx_result cx_gfx_shader_program_build_from_source(
	const struct cx_gfx_shader_program* p_shader_program, const struct cx_shader_source* p_source);
void cx_gfx_shader_program_reflect_interface(
	const struct cx_gfx_shader_program* p_shader_program, struct cx_gfx_shader_program_interface* p_out);
void cx_gfx_shader_program_free_interface(struct cx_gfx_shader_program_interface* p_interface);
void cx_gfx_shader_program_bind(const struct cx_gfx_shader_program* p_shader_program);

#endif
