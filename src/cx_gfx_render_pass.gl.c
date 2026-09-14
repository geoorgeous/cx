#include <string.h>

#include "gl.h"

#include "cx_dbg.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_render_pass.h"
#include "cx_gfx_shader_program.h"
#include "cx_gfx_shader_program_interface.gl.h"
#include "cx_gfx_texture.h"
#include "cx_gfx_texture.gl.h"
#include "cx_logging.h"
#include "cx_render_pipeline.h"
#include "cx_shader.h"
#include "math_utils.h"

#define CX_GFX_RENDER_PASS_UBO_SIZE 1024

static char g_ubo_staging_buffer[CX_GFX_RENDER_PASS_UBO_SIZE];
static size_t g_ubo_staging_buffer_size;
static size_t g_ubo_staging_buffer_block_offsets[16];
static GLuint g_gl_ubo;
static size_t g_block_alignment;

static void cx_gfx_render_pass_apply_state(const struct cx_gfx_render_pass* p_render_pass);

static void cx_gfx_render_pass_set_active_shader(const struct cx_shader* p_shader);

static void cx_gfx_render_pass_apply_pipeline_state(const struct cx_render_pipeline_state* p_render_pipeline_state);

static int cx_render_pipeline_state_cmp(
	const struct cx_render_pipeline_state* p_a, const struct cx_render_pipeline_state* p_b);

static void upload_shader_program_input_set(
	const struct cx_gfx_shader_program_interface* p_shader_program_interface,
	const struct cx_gfx_shader_program_input_set* p_input_set);

static void upload_shader_program_parameter(
	const struct cx_gfx_shader_program_parameter_info* p_parameter, const void* p_data);

static void upload_shader_program_texture(
	const struct cx_gfx_shader_program_texture_info* p_texture_info, const struct cx_gfx_texture* p_texture);

static void stage_shader_program_block(
	const struct cx_gfx_shader_program_block_info* p_block_info, const void* p_data, size_t data_size);

void cx_gfx_render_pass_execute(
	const struct cx_gfx_render_pass* p_render_pass,
	struct cx_render_draw_command* p_draw_commands,
	uint32_t num_draw_commands) {

	cx_gfx_render_pass_apply_state(p_render_pass);

	const struct cx_shader* p_active_shader = CX_NULL;
	struct cx_render_pipeline_state current_render_pipeline_state = {0};

	for (uint32_t i = 0; i < num_draw_commands; ++i) {
		const struct cx_render_draw_command* p_draw_command = &p_draw_commands[i];

		if (p_draw_command->pipeline.p_shader != p_active_shader) {
			cx_gfx_render_pass_apply_pipeline_state(&p_draw_command->pipeline.state);
			current_render_pipeline_state = p_draw_command->pipeline.state;

			cx_gfx_render_pass_set_active_shader(p_draw_command->pipeline.p_shader);
			p_active_shader = p_draw_command->pipeline.p_shader;

			upload_shader_program_input_set(
				&p_active_shader->gfx_program_interface_, &p_render_pass->pass_input_set);

			upload_shader_program_input_set(
				&p_active_shader->gfx_program_interface_, &p_draw_command->material_input_set);
		} else {
			if (!cx_render_pipeline_state_cmp(&current_render_pipeline_state, &p_draw_command->pipeline.state)) {
				cx_gfx_render_pass_apply_pipeline_state(&p_draw_command->pipeline.state);
				current_render_pipeline_state = p_draw_command->pipeline.state;
			}

			upload_shader_program_input_set(
				&p_active_shader->gfx_program_interface_, &p_draw_command->material_input_set);
		}

		upload_shader_program_input_set(
			&p_active_shader->gfx_program_interface_, &p_draw_command->draw_input_set);
		
		glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)g_ubo_staging_buffer_size, g_ubo_staging_buffer);

		cx_gfx_mesh_draw(p_draw_command->p_mesh);
	}
}

void cx_gfx_render_pass_apply_state(const struct cx_gfx_render_pass* p_render_pass) {
	cx_gfx_framebuffer_bind(p_render_pass->p_framebuffer);

	glViewport(
		(GLint)p_render_pass->viewport[0], 
		(GLint)p_render_pass->viewport[1],
		(GLint)p_render_pass->viewport[2],
		(GLint)p_render_pass->viewport[3]);

	if (p_render_pass->clear_flags != CX_GFX_RENDER_TARGET_CLEAR_FLAG_none) {
		GLbitfield gl_clear_mask = 0;

		if (p_render_pass->clear_flags & CX_GFX_RENDER_TARGET_CLEAR_FLAG_color) {
			glClearColor(
				(GLfloat)p_render_pass->clear_color[0],
				(GLfloat)p_render_pass->clear_color[1],
				(GLfloat)p_render_pass->clear_color[2],
				(GLfloat)p_render_pass->clear_color[3]);
			gl_clear_mask |= GL_COLOR_BUFFER_BIT;
		}

		if (p_render_pass->clear_flags & CX_GFX_RENDER_TARGET_CLEAR_FLAG_depth) {
			glClearDepth((GLdouble)p_render_pass->clear_depth);
			gl_clear_mask |= GL_DEPTH_BUFFER_BIT;
		}

		if (p_render_pass->clear_flags & CX_GFX_RENDER_TARGET_CLEAR_FLAG_stencil) {
			glClearStencil((GLint)p_render_pass->clear_stencil);
			gl_clear_mask |= GL_STENCIL_BUFFER_BIT;
		}

		glClear(gl_clear_mask);
	}
}

void cx_gfx_render_pass_set_active_shader(const struct cx_shader* p_shader) {
	CX_ASSERT(p_shader != NULL, GFX_RENDER_PASS);

	if (g_gl_ubo == 0) {
		GLint offset_alignment;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &offset_alignment);
		g_block_alignment = (size_t)offset_alignment;

		glGenBuffers(1, &g_gl_ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, g_gl_ubo);
		glBufferData(GL_UNIFORM_BUFFER, CX_GFX_RENDER_PASS_UBO_SIZE, CX_NULL, GL_DYNAMIC_DRAW);
	} else {
		glBindBuffer(GL_UNIFORM_BUFFER, g_gl_ubo);
	}

	g_ubo_staging_buffer_size = 0;

	const struct cx_gfx_shader_program_interface* p_interface = &p_shader->gfx_program_interface_;

	for (size_t i = 0; i < p_interface->num_blocks; ++i) {
		const struct cx_gfx_shader_program_block_info* p_block_info =
			&p_interface->p_blocks[i];
		const struct cx_gfx_shader_program_block_info_gl_internals* p_block_info_internals =
			CX_GET_OPAQUE_INTERNALS_CONST(*p_block_info);

		const size_t offset_misalignment = g_ubo_staging_buffer_size % g_block_alignment;
		if (offset_misalignment != 0) {
			g_ubo_staging_buffer_size += g_block_alignment - offset_misalignment;
		}

		g_ubo_staging_buffer_block_offsets[p_block_info_internals->uniform_block_binding_point] =
			g_ubo_staging_buffer_size;

		glBindBufferRange(
			GL_UNIFORM_BUFFER,
			p_block_info_internals->uniform_block_binding_point,
			g_gl_ubo,
			(GLintptr)g_ubo_staging_buffer_size,
			(GLintptr)p_block_info->size);

		CX_LOG_FMT(TRACE, GFX_RENDER_PASS,
			"Binding UBO region: offset=%"CX_PRI_SIZE", size=%"CX_PRI_SIZE", opengl_binding_point=%u\n",
			g_ubo_staging_buffer_size, p_block_info->size, p_block_info_internals->uniform_block_binding_point);

		g_ubo_staging_buffer_size += p_block_info->size;
	}

	cx_gfx_shader_program_bind(&p_shader->gfx_program_);
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

void cx_gfx_render_pass_apply_pipeline_state(const struct cx_render_pipeline_state* p_render_pipeline_state) {
	if (p_render_pipeline_state->flags & CX_RENDER_PIPELINE_FLAG_depth_test_enabled) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(g_gl_depth_funcs[p_render_pipeline_state->depth_test_func]);
		glDepthMask(!!(p_render_pipeline_state->flags & CX_RENDER_PIPELINE_FLAG_depth_writes_enabled));
	} else {
		glDisable(GL_DEPTH_TEST);
	}

	if (p_render_pipeline_state->flags & CX_RENDER_PIPELINE_FLAG_blend_enabled) {
		glEnable(GL_BLEND);

		glBlendFunc(
			g_gl_blend_funcs[p_render_pipeline_state->blend_src_func],
			g_gl_blend_funcs[p_render_pipeline_state->blend_dst_func]);

		if ((p_render_pipeline_state->blend_src_func >= CX_BLEND_FUNC_blend_color &&
			p_render_pipeline_state->blend_src_func <= CX_BLEND_FUNC_one_minus_blend_color_alpha) ||
			(p_render_pipeline_state->blend_dst_func >= CX_BLEND_FUNC_blend_color &&
			p_render_pipeline_state->blend_dst_func <= CX_BLEND_FUNC_one_minus_blend_color_alpha)) {

			glBlendColor(
				(GLfloat)p_render_pipeline_state->blend_color[0],
				(GLfloat)p_render_pipeline_state->blend_color[1],
				(GLfloat)p_render_pipeline_state->blend_color[2],
				(GLfloat)p_render_pipeline_state->blend_color[3]);
		}
	} else {
		glDisable(GL_BLEND);
	}

	if (p_render_pipeline_state->cull_mode == CX_CULL_MODE_none) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
		glCullFace(g_gl_cull_face_modes[p_render_pipeline_state->cull_mode]);
		glFrontFace(
			p_render_pipeline_state->flags & CX_RENDER_PIPELINE_FLAG_front_face_clockwise_ordering_enabled ?
			GL_CW :
			GL_CCW);
	}
}

int cx_render_pipeline_state_cmp(
	const struct cx_render_pipeline_state* p_a, const struct cx_render_pipeline_state* p_b) {

	return
		p_a->flags == p_b->flags &&
		p_a->depth_test_func == p_b->depth_test_func &&
		p_a->blend_src_func == p_b->blend_src_func &&
		p_a->blend_dst_func == p_b->blend_dst_func &&
		p_a->cull_mode == p_b->cull_mode &&
		FLT_CMP(p_a->blend_color[0], p_b->blend_color[0]) &&
		FLT_CMP(p_a->blend_color[1], p_b->blend_color[1]) &&
		FLT_CMP(p_a->blend_color[2], p_b->blend_color[2]) &&
		FLT_CMP(p_a->blend_color[3], p_b->blend_color[3]);
}

static const struct cx_gfx_shader_program_parameter_info* get_shader_program_prameter_info_by_name(
	const struct cx_gfx_shader_program_interface* p_interface, const char* s_name) {
	
	for (uint16_t i = 0; i < p_interface->num_parameters; ++i) {
		if (strcmp(p_interface->p_parameters[i].name, s_name) == 0) {
			return &p_interface->p_parameters[i];
		}
	}

	return CX_NULL;
}

static const struct cx_gfx_shader_program_texture_info* get_shader_program_texture_info_by_name(
	const struct cx_gfx_shader_program_interface* p_interface, const char* s_name) {
	
	for (uint16_t i = 0; i < p_interface->num_textures; ++i) {
		if (strcmp(p_interface->p_textures[i].name, s_name) == 0) {
			return &p_interface->p_textures[i];
		}
	}

	return CX_NULL;
}

static const struct cx_gfx_shader_program_block_info* get_shader_program_block_info_by_name(
	const struct cx_gfx_shader_program_interface* p_interface, const char* s_name) {
	
	for (uint16_t i = 0; i < p_interface->num_blocks; ++i) {
		if (strcmp(p_interface->p_blocks[i].name, s_name) == 0) {
			return &p_interface->p_blocks[i];
		}
	}

	return CX_NULL;
}

void upload_shader_program_input_set(
	const struct cx_gfx_shader_program_interface* p_shader_program_interface,
	const struct cx_gfx_shader_program_input_set* p_input_set) {

	for (uint16_t i = 0; i < p_input_set->num_parameters; ++i) {
		const struct cx_gfx_shader_program_parameter_info* p_info =
			get_shader_program_prameter_info_by_name(p_shader_program_interface, p_input_set->p_parameters[i].s_name);

		if (p_info == CX_NULL) {
			CX_LOG_FMT(TRACE, GFX_RENDER_PASS, "Couldn't upload shader program parameter '%s': Not found\n",
				p_input_set->p_parameters[i].s_name);
			continue;
		}

		if (p_info->type != p_input_set->p_parameters[i].type) {
			CX_LOG_FMT(TRACE, GFX_RENDER_PASS,
				"Couldn't upload shader program parameter '%s': Type mismatch (expected %d, got %d)\n",
				p_input_set->p_parameters[i].s_name, p_info->type, p_input_set->p_parameters[i].type);
			continue;
		}

		upload_shader_program_parameter(p_info, p_input_set->p_parameters[i].p_value);
	}

	for (uint16_t i = 0; i < p_input_set->num_textures; ++i) {
		const struct cx_gfx_shader_program_texture_info* p_info =
			get_shader_program_texture_info_by_name(p_shader_program_interface, p_input_set->p_textures[i].s_name);

		if (p_info == CX_NULL) {
			CX_LOG_FMT(TRACE, GFX_RENDER_PASS, "Couldn't upload shader program texture '%s': Not found\n",
				p_input_set->p_textures[i].s_name);
			continue;
		}

		upload_shader_program_texture(p_info, p_input_set->p_textures[i].p_texture);
	}

	for (uint16_t i = 0; i < p_input_set->num_blocks; ++i) {
		const struct cx_gfx_shader_program_block_info* p_info =
			get_shader_program_block_info_by_name(p_shader_program_interface, p_input_set->p_blocks[i].s_name);

		if (p_info == CX_NULL) {
			CX_LOG_FMT(TRACE, GFX_RENDER_PASS, "Couldn't upload shader program block '%s': Not found\n",
				p_input_set->p_blocks[i].s_name);
			continue;
		}

		stage_shader_program_block(p_info, p_input_set->p_blocks[i].p_data, p_input_set->p_blocks[i].size);
	}
}

static inline void upload_shader_program_parameter_u32(GLint uniform_location, const void* p_value) {
	glUniform1ui(uniform_location, *((const GLuint*)p_value));
}

static inline void upload_shader_program_parameter_i32(GLint uniform_location, const void* p_value) {
	glUniform1i(uniform_location, *((const GLint*)p_value));
}

static inline void upload_shader_program_parameter_f32(GLint uniform_location, const void* p_value) {
	glUniform1f(uniform_location, *((const GLfloat*)p_value));
}

static inline void upload_shader_program_parameter_uvec2(GLint uniform_location, const void* p_value) {
	glUniform2uiv(uniform_location, 1, (const GLuint*)p_value);
}

static inline void upload_shader_program_parameter_ivec2(GLint uniform_location, const void* p_value) {
	glUniform2iv(uniform_location, 1, (const GLint*)p_value);
}

static inline void upload_shader_program_parameter_fvec2(GLint uniform_location, const void* p_value) {
	glUniform2fv(uniform_location, 1, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_uvec3(GLint uniform_location, const void* p_value) {
	glUniform3uiv(uniform_location, 1, (const GLuint*)p_value);
}

static inline void upload_shader_program_parameter_ivec3(GLint uniform_location, const void* p_value) {
	glUniform3iv(uniform_location, 1, (const GLint*)p_value);
}

static inline void upload_shader_program_parameter_fvec3(GLint uniform_location, const void* p_value) {
	glUniform3fv(uniform_location, 1, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_uvec4(GLint uniform_location, const void* p_value) {
	glUniform4uiv(uniform_location, 1, (const GLuint*)p_value);
}

static inline void upload_shader_program_parameter_ivec4(GLint uniform_location, const void* p_value) {
	glUniform4iv(uniform_location, 1, (const GLint*)p_value);
}

static inline void upload_shader_program_parameter_fvec4(GLint uniform_location, const void* p_value) {
	glUniform4fv(uniform_location, 1, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat2(GLint uniform_location, const void* p_value) {
	glUniformMatrix2fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat3(GLint uniform_location, const void* p_value) {
	glUniformMatrix3fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat4(GLint uniform_location, const void* p_value) {
	glUniformMatrix4fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat2x3(GLint uniform_location, const void* p_value) {
	glUniformMatrix2x3fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat2x4(GLint uniform_location, const void* p_value) {
	glUniformMatrix2x4fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat3x2(GLint uniform_location, const void* p_value) {
	glUniformMatrix3x2fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat3x4(GLint uniform_location, const void* p_value) {
	glUniformMatrix3x4fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat4x2(GLint uniform_location, const void* p_value) {
	glUniformMatrix4x2fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

static inline void upload_shader_program_parameter_mat4x3(GLint uniform_location, const void* p_value) {
	glUniformMatrix4x3fv(uniform_location, 1, GL_FALSE, (const GLfloat*)p_value);
}

void upload_shader_program_parameter(
	const struct cx_gfx_shader_program_parameter_info* p_parameter_info, const void* p_value) {

	static void(*const func_table[])(GLint, const void*) = {
		upload_shader_program_parameter_i32,
		upload_shader_program_parameter_u32,
		upload_shader_program_parameter_i32,
		upload_shader_program_parameter_f32,
		upload_shader_program_parameter_uvec2,
		upload_shader_program_parameter_ivec2,
		upload_shader_program_parameter_fvec2,
		upload_shader_program_parameter_uvec3,
		upload_shader_program_parameter_ivec3,
		upload_shader_program_parameter_fvec3,
		upload_shader_program_parameter_uvec4,
		upload_shader_program_parameter_ivec4,
		upload_shader_program_parameter_fvec4,
		upload_shader_program_parameter_mat2,
		upload_shader_program_parameter_mat3,
		upload_shader_program_parameter_mat4,
		upload_shader_program_parameter_mat2x3,
		upload_shader_program_parameter_mat2x4,
		upload_shader_program_parameter_mat3x2,
		upload_shader_program_parameter_mat3x4,
		upload_shader_program_parameter_mat4x2,
		upload_shader_program_parameter_mat4x3
	};

	const struct cx_gfx_shader_program_parameter_info_gl_internals* p_parameter_info_internals =
		CX_GET_OPAQUE_INTERNALS_CONST(*p_parameter_info);

	func_table[p_parameter_info->type](p_parameter_info_internals->uniform_location, p_value);
}

void upload_shader_program_texture(
	const struct cx_gfx_shader_program_texture_info* p_texture_info, const struct cx_gfx_texture* p_texture) {

	const struct cx_gfx_shader_program_texture_info_gl_internals* p_texture_info_internals =
		CX_GET_OPAQUE_INTERNALS_CONST(*p_texture_info);

	const struct cx_gfx_texture_gl_internals* p_texture_internals =
		CX_GET_OPAQUE_INTERNALS_CONST(*p_texture);

	CX_ASSERT(p_texture_internals->id != 0, GFX_SHADER_PROGRAM);

	glActiveTexture(GL_TEXTURE0 + (GLenum)p_texture_info_internals->texture_unit);
	glBindTexture(p_texture_info_internals->texture_target, p_texture_internals->id);
}

void stage_shader_program_block(
	const struct cx_gfx_shader_program_block_info* p_block_info, const void* p_data, size_t data_size) {

	CX_ASSERT(p_data != CX_NULL, GFX_RENDER_PASS);
	CX_ASSERT(data_size != 0, GFX_RENDER_PASS);

	const struct cx_gfx_shader_program_block_info_gl_internals* p_info_internals =
		CX_GET_OPAQUE_INTERNALS_CONST(*p_block_info);

	const size_t staging_buffer_block_offset =
		g_ubo_staging_buffer_block_offsets[p_info_internals->uniform_block_binding_point];

	const size_t size = CX_MATH_MIN(p_block_info->size, data_size);

	CX_LOG_FMT(TRACE, GFX_RENDER_PASS,
		"Staging block data: name='%s', offset=%"CX_PRI_SIZE", size=%"CX_PRI_SIZE", p_data=%p\n",
		p_block_info->name, staging_buffer_block_offset, size, p_data);

	memcpy(g_ubo_staging_buffer + staging_buffer_block_offset, p_data, size);
}
