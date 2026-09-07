#include <string.h>

#include "gl.h"

#include "cx_dbg.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_texture.h"
#include "cx_gfx_texture.gl.h"
#include "cx_shader.h"
#include "cx_shader.gl.h"
#include "cx_material.h"
#include "cx_render_pass.h"
#include "cx_render_pipeline.h"

#define CX_RENDER_PASS_UBO_SIZE 1024

static char g_ubo_staging_buffer[CX_RENDER_PASS_UBO_SIZE];
static size_t g_ubo_staging_buffer_size;
static size_t g_ubo_staging_buffer_block_offsets[16];
static GLuint g_gl_ubo;

static void cx_render_pass_bind_pipeline(
	const struct cx_render_pass* p_render_pass, const struct cx_render_pipeline* p_render_pipeline);

static void cx_render_pass_upload_render_params_to_shader(
	const struct cx_render_param* p_params, uint16_t num_params, const struct cx_shader* p_shader);

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

	const struct cx_shader* p_current_shader = CX_NULL;
	const struct cx_material* p_current_material = CX_NULL;

	// For each parameter set:
	//    For each parameter:
	//        Try and find the binding location in the current shader (we should match ID and type)
	//        If we find a matching binding:
	//            If it is simple param:
	//                Send uniform/push constant to shader using the binding location (uniform location)
	//            Else if it is an opaque:
	//                Set the active unit/slot to the one identified in the binding we got from the shader
	//                Bind the resource to the active unit/slot
	//            Else if it is a block:
	//                Copy the parameter block data in to the UBO
	//                Bind the UBO to the binding point identified by the binding we got from the shader
	//                Bind the parameter UBO range to the binding point

	for (uint32_t i = 0; i < num_draw_commands; ++i) {
		const struct cx_render_draw_command* p_draw_command = &p_draw_commands[i];

		if (p_draw_command->p_material->render_pipeline.shader_asset_ref != p_bound_pipeline) {
			cx_render_pass_bind_pipeline(p_render_pass, p_draw_command->p_material->p_render_pipeline);

			cx_render_pass_upload_render_params_to_shader(
				p_draw_command->p_material->param_set.p_params,
				p_draw_command->p_material->param_set.num_params,
				p_draw_command->p_material->render_pipeline.p_shader);

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
		
		cx_render_pass_upload_render_params_to_shader(
			p_draw_command->p_param_set->p_params,
			p_draw_command->p_param_set->num_params,
			p_bound_pipeline->p_shader);
		
		glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)g_ubo_staging_buffer_size, g_ubo_staging_buffer);

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
	
	if (p_render_pipeline->flags & CX_RENDER_PIPELINE_FLAG_depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(g_gl_depth_funcs[p_render_pipeline->depth_test_func]);
		glDepthMask(p_render_pipeline->flags & CX_RENDER_PIPELINE_FLAG_depth_writes_enabled);
	} else {
		glDisable(GL_DEPTH_TEST);
	}

	if (p_render_pipeline->flags & CX_RENDER_PIPELINE_FLAG_blend_enabled) {
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
		glFrontFace(
			p_render_pipeline->flags & CX_RENDER_PIPELINE_FLAG_front_face_clockwise_ordering_enabled ?
			GL_CW :
			GL_CCW);
	}

	// Construct/bind UBO

	if (g_gl_ubo == 0) {
		glGenBuffers(1, &g_gl_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, g_gl_ubo);
		glBufferData(GL_UNIFORM_BUFFER, CX_RENDER_PASS_UBO_SIZE, CX_NULL, GL_STATIC_DRAW);
	} else {
		glBindBuffer(GL_UNIFORM_BUFFER, g_gl_ubo);
	}

	// Map pipeline shader blocks to staging buffer and UBO

	g_ubo_staging_buffer_size = 0;

	for (size_t i = 0; i < p_render_pipeline->p_shader->num_block_bindings; ++i) {
		const struct cx_shader_binding_block_gl_internals* p_block_binding = 
			(const void*)p_render_pipeline->p_shader->p_block_bindings[i].internals_.bytes_;

		g_ubo_staging_buffer_block_offsets[p_block_binding->uniform_block_binding_point] = g_ubo_staging_buffer_size;

		glBindBufferRange(
			GL_UNIFORM_BUFFER,
			p_block_binding->uniform_block_binding_point,
			g_gl_ubo,
			(GLintptr)g_ubo_staging_buffer_size,
			(GLintptr)p_render_pipeline->p_shader->p_block_bindings[i].size);

		g_ubo_staging_buffer_size += p_render_pipeline->p_shader->p_block_bindings[i].size;
	}

	cx_shader_bind(p_render_pipeline->p_shader);

	cx_render_pass_upload_render_params_to_shader(
		p_render_pass->param_set.p_params,
		p_render_pass->param_set.num_params,
		p_render_pipeline->p_shader);

	// every time a shader is bound, we query it for what uniform blocks it has, then we map out the blocks on to the 
	// uniform buffer. then we keep track of where each block sits in the buffer.
	// then, when someone comes along to set that buffer data, we just place the data in the region of the buffer we mapped out
	//
	// so our shader just needs to tell us:
	//
	// what blocks do you expect, and what are the sizes?
	//
	// then later, given this parameter ID, tell us how to bind this parameter to you

	// todo: bind per-pass shader data
	//
	// TODO: shader metadata for parameter bindings: uniforms, blocks, block members, samplers
	// TODO: shader interface for telling the renderer about its blocks
	//
	// param_bindings[] // opengl uniform location + type
	// param_block_bindings[] // opengl binding_point + size
	// param_block_member_bindings[] // opengl binding_point + offset + size + type
	// sampler_binding[] // opengl texture unit
	
	// For each parameter set:
	//    For each parameter:
	//        Try and find the binding location in the current shader (we should match ID and type)
	//        If we find a matching binding:
	//            If it is simple param:
	//                Send uniform/push constant to shader using the binding location (uniform location)
	//            Else if it is an opaque:
	//                Set the active unit/slot to the one identified in the binding we got from the shader
	//                Bind the resource to the active unit/slot
	//            Else if it is a block:
	//                Copy the parameter block data in to the UBO
	//                Bind the UBO to the binding point identified by the binding we got from the shader
	//                Bind the parameter UBO range to the binding point
}

void cx_render_pass_upload_render_params_to_shader(
	const struct cx_render_param* p_params, uint16_t num_params, const struct cx_shader* p_shader) {

	for (uint16_t i = 0; i < num_params; ++i) {
		const struct cx_render_param* p_render_param = &p_params[i];

		const struct cx_shader_binding* p_binding;
		if (!cx_shader_find_binding(p_shader, p_render_param->s_name, &p_binding)) {
			CX_LOG(WARNING, RENDER_PASS, "No binding found for render parameter\n");
		}

		// TODO: make sure binding matches parameter type
		
		switch (p_binding->type) {
			case CX_SHADER_BINDING_TYPE_param: {
				const struct cx_shader_binding_param_gl_internals* p_binding_internals =
					(const void*)p_binding->data.param.internals_.bytes_;

				const GLint loc = p_binding_internals->uniform_location;

				switch (p_binding->data.param.type) {
					case CX_SHADER_PARAM_TYPE_u32: {
						glUniform1ui(loc, (const GLuint)*(const uint32_t*)p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_i32: { 
						glUniform1i(loc, (const GLint)*(const int32_t*)p_render_param->p_data);
						break; 
					}

					case CX_SHADER_PARAM_TYPE_f32: {
						glUniform1f(loc, (const GLfloat)*(const float*)p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_uvec2: {
						const uint32_t* p_vec = p_render_param->p_data;
						glUniform2ui(loc, (GLuint)p_vec[0], (GLuint)p_vec[1]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_ivec2: {
						const int32_t* p_vec = p_render_param->p_data;
						glUniform2i(loc, (GLint)p_vec[0], (GLint)p_vec[1]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_fvec2: {
						const float* p_vec = p_render_param->p_data;
						glUniform2f(loc, (GLfloat)p_vec[0], (GLfloat)p_vec[1]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_uvec3: {
						const uint32_t* p_vec = p_render_param->p_data;
						glUniform3ui(loc, (GLuint)p_vec[0], (GLuint)p_vec[1], (GLuint)p_vec[2]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_ivec3: {
						const int32_t* p_vec = p_render_param->p_data;
						glUniform3i(loc, (GLint)p_vec[0], (GLint)p_vec[1], (GLint)p_vec[2]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_fvec3: {
						const float* p_vec = p_render_param->p_data;
						glUniform3f(loc, (GLfloat)p_vec[0], (GLfloat)p_vec[1], (GLfloat)p_vec[2]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_uvec4: {
						const uint32_t* p_vec = p_render_param->p_data;
						glUniform4ui(loc, (GLuint)p_vec[0], (GLuint)p_vec[1], (GLuint)p_vec[2], (GLuint)p_vec[3]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_ivec4: {
						const int32_t* p_vec = p_render_param->p_data;
						glUniform4i(loc, (GLint)p_vec[0], (GLint)p_vec[1], (GLint)p_vec[2], (GLint)p_vec[3]);
						break;
					}

					case CX_SHADER_PARAM_TYPE_fvec4: {
						const float* p_vec = p_render_param->p_data;
						glUniform4f(loc, (GLfloat)p_vec[0], (GLfloat)p_vec[1], (GLfloat)p_vec[2], (GLfloat)p_vec[3]);
						break;
					}


					case CX_SHADER_PARAM_TYPE_mat2: {
						glUniformMatrix2fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat3: {
						glUniformMatrix3fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat4: {
						glUniformMatrix4fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat2x3: {
						glUniformMatrix2x3fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat2x4: {
						glUniformMatrix2x4fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat3x2: {
						glUniformMatrix3x2fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat3x4: {
						glUniformMatrix3x4fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat4x2: {
						glUniformMatrix4x2fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}

					case CX_SHADER_PARAM_TYPE_mat4x3: {
						glUniformMatrix4x3fv(loc, 1, GL_FALSE, p_render_param->p_data);
						break;
					}
				}

				break;
			}

			case CX_SHADER_BINDING_TYPE_block: {
				const struct cx_shader_binding_block_gl_internals* p_binding_internals =
					(const void*)p_binding->data.block.internals_.bytes_;

				const size_t staging_buffer_block_offset =
					g_ubo_staging_buffer_block_offsets[p_binding_internals->uniform_block_binding_point];

				memcpy(
					g_ubo_staging_buffer + staging_buffer_block_offset,
					p_render_param->p_data,
					p_binding->data.block.size);

				break;
			}

			case CX_SHADER_BINDING_TYPE_block_member: {
				const struct cx_shader_binding_block_member_gl_internals* p_binding_internals =
					(const void*)p_binding->data.block_member.internals_.bytes_;

				const size_t staging_buffer_block_offset =
					g_ubo_staging_buffer_block_offsets[p_binding_internals->uniform_block_binding_point];

				memcpy(
					g_ubo_staging_buffer + staging_buffer_block_offset + p_binding->data.block_member.offset,
					p_render_param->p_data,
					p_binding->data.block_member.size);

				break;
			}

			case CX_SHADER_BINDING_TYPE_sampler: {
				const struct cx_shader_binding_sampler_gl_internals* p_internals =
					(const void*)p_binding->data.sampler.internals_.bytes_;

				const struct cx_gfx_texture* p_texture = p_render_param->p_data;
				const struct cx_gfx_texture_gl_internals* p_texture_internals = (const void*)p_texture->bytes_;

				glActiveTexture((GLenum)((GLint)GL_TEXTURE0 + p_internals->texture_unit));
				glBindTexture(p_internals->texture_target, p_texture_internals->id);
				
				break;
			}
		}
	}
}
