#include "gl.h"

#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"
#include "cx_material.h"
#include "cx_render_pass.h"
#include "cx_render_pipeline.h"

static void cx_render_pass_bind_pipeline(
	const struct cx_render_pass* p_render_pass, const struct cx_render_pipeline* p_render_pipeline);

void cx_render_pass_execute(
	const struct cx_render_pass* p_render_pass,
	struct cx_render_draw_command* p_draw_commands,
	uint32_t num_draw_commands) {

	cx_gfx_framebuffer_bind(p_render_pass->p_framebuffer);

	glViewport(
		(GLint)p_render_pass->viewport[0], 
		(GLint)p_render_pass->viewport[1],
		(GLint)p_render_pass->viewport[2],
		(GLint)p_render_pass->viewport[3]);

	if (p_render_pass->clear_mask != CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_none) {
		GLbitfield gl_clear_mask = 0;
		if (p_render_pass->clear_mask & CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_color) {
			glClearColor(
				(GLfloat)p_render_pass->clear_color[0],
				(GLfloat)p_render_pass->clear_color[1],
				(GLfloat)p_render_pass->clear_color[2],
				(GLfloat)p_render_pass->clear_color[3]);
			gl_clear_mask |= GL_COLOR_BUFFER_BIT;
		}
		if (p_render_pass->clear_mask & CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_depth) {
			glClearDepth((GLdouble)p_render_pass->clear_depth);
			gl_clear_mask |= GL_DEPTH_BUFFER_BIT;
		}
		if (p_render_pass->clear_mask & CX_GFX_RENDER_TARGET_CLEAR_BIT_MASK_stencil) {
			glClearStencil((GLint)p_render_pass->clear_stencil);
			gl_clear_mask |= GL_STENCIL_BUFFER_BIT;
		}
		glClear(gl_clear_mask);
	}

	const struct cx_render_pipeline* p_bound_pipeline = CX_NULL;

	for (uint32_t i = 0; i < num_draw_commands; ++i) {
		const struct cx_render_draw_command* p_draw_command = &p_draw_commands[i];

		if (p_draw_command->p_material->p_render_pipeline != p_bound_pipeline) {
			cx_render_pass_bind_pipeline(p_render_pass, p_draw_command->p_material->p_render_pipeline);
			p_bound_pipeline = p_draw_command->p_material->p_render_pipeline;
		}
		
		// todo: bind material shader data if different from last

		// for simple uniforms: glUniform*
		// for opaque resources: glUniform1i + glBindTexture + glActiveTexture
		// for structured data: glUniformBlockBinding + glBindBuffer + glBufferData/SubData + glBindBufferBase/Range
		
		// OPAQUE RESOURCES
		//
		// done at shader init:
		// glUniform1i(opaque_resource_loc, n): tell the shader that opaque_resource gets its resource from texture unit n
		//
		// done by renderer:
		// glActiveTexture(n): make texture unit n the active texture unit to operate on
		// glBindTexture(id): make texture with this id bound to the active texture unit 
		//
		// renderer can keep track of texture_unit bound textures to (per texture target)
	
		// UNIFORM BLOCKS
		//
		// done at shader init:
		// glUniformBlockBinding(block_index, n): tell the shader than block this this index gets its data from UBO binding point n
		//
		// done by renderer:
		// glBindBufferBase(bp, ubo): tell the binding_point bp that its UBO is ubo
		// glBindBuffer(ubo): make UBO ubo the active UBO to operate on
		// glBufferData(data): upload data to the active UBO
		//
		// renderer can keep track of binding_point bound UBOs

		// todo: bind per-draw command shader data

		cx_gfx_mesh_draw(p_draw_command->p_mesh);
	}
}

const static GLenum g_gl_depth_funcs[] = {
	GL_NEVER,
	GL_ALWAYS,
	GL_EQUAL,
	GL_NOTEQUAL,
	GL_LESS,
	GL_LEQUAL,
	GL_GREATER,
	GL_GEQUAL
};

const static GLenum g_gl_blend_funcs[] = {
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_CONSTANT_COLOR,
	GL_ONE_MINUS_CONSTANT_COLOR,
	GL_CONSTANT_ALPHA,
	GL_ONE_MINUS_CONSTANT_ALPHA
};

const static GLenum g_gl_cull_face_modes[] = {
	0,
	GL_BACK,
	GL_FRONT,
	GL_FRONT_AND_BACK
};

void cx_render_pass_bind_pipeline(
	const struct cx_render_pass* p_render_pass, const struct cx_render_pipeline* p_render_pipeline) {
	
	if (p_render_pipeline->b_depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(g_gl_depth_funcs[p_render_pipeline->depth_test_func]);
		glDepthMask(p_render_pipeline->b_depth_writes_enabled ? GL_TRUE : GL_FALSE);
	} else {
		glDisable(GL_DEPTH_TEST);
	}

	if (p_render_pipeline->b_blend_enabled) {
		glEnable(GL_BLEND);

		glBlendFunc(
			g_gl_blend_funcs[p_render_pipeline->blend_src_func],
			g_gl_blend_funcs[p_render_pipeline->blend_dst_func]);

		if ((p_render_pipeline->blend_src_func >= CX_BLEND_FUNC_blend_color &&
			p_render_pipeline->blend_src_func <= CX_BLEND_FUNC_one_minus_blend_color) ||
			(p_render_pipeline->blend_dst_func >= CX_BLEND_FUNC_blend_color &&
			p_render_pipeline->blend_dst_func <= CX_BLEND_FUNC_one_minus_blend_color)) {

			glBlendColor(
				(GLfloat)p_render_pipeline->blend_color[0],
				(GLfloat)p_render_pipeline->blend_color[1],
				(GLfloat)p_render_pipeline->blend_color[2],
				(GLfloat)p_render_pipeline->blend_color[3]);
		}
	} else {
		glDisable(GL_BLEND);
	}

	if (p_render_pipeline->cull_mode == CX_CULL_MODE_none) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
		glCullFace(g_gl_cull_face_modes[p_render_pipeline->cull_mode]);
		glFrontFace(p_render_pipeline->b_enable_front_face_clockwise_ordering ? GL_CW : GL_CCW);
	}
	
	cx_gfx_program_bind(p_render_pipeline->p_program);

	// todo: bind per-pass shader data
}
