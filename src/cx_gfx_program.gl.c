#include <string.h>

#include "cx_gfx_program.h"
#include "cx_dbg.h"
#include "cx_gfx_texture.h"
#include "cx_gfx_texture.gl.h"
#include "cx_error.h"
#include "gl.h"
#include "cx_logging.h"

#define UNIFORM_NAME_MAX_LEN 64
#define UNIFORM_BLOCK_MAX_UNIFORMS 64

struct cx_gfx_program_param_block_gl_internals {
	GLint bind_index;
};

struct cx_gfx_program_param_buffer_gl_internals {
	GLuint id;
};

struct cx_gfx_program_gl_internals {
	GLuint id;
};

static enum cx_error compile_shader_source(GLuint shader, const char* s_source);
void              log_program_info_log(GLuint program, int level, const char* message);
static GLint      get_uniform_index(GLuint program, const char* s_name);

enum cx_error cx_gfx_program_param_buffer_create(struct cx_gfx_program_param_buffer* p_buffer, size_t size) {
	struct cx_gfx_program_param_buffer_gl_internals* p_buffer_internals = (void*)p_buffer->bytes_;

	glGenBuffers(1, &p_buffer_internals->id);

	if (p_buffer_internals->id == 0) {
		return CX_ERROR_allocation_failed;
	}

	glBindBuffer(GL_UNIFORM_BUFFER, p_buffer_internals->id);
	glBufferData(GL_UNIFORM_BUFFER, (GLsizeiptr)size, 0, GL_DYNAMIC_DRAW);

	p_buffer->size = size;

	CX_DBG(CX_LOG_FMT(TRACE, GFX_PROGRAM,
		"Program param buffer created: opengl_buffer_id=%d, size=%u\n",
		p_buffer_internals->id, p_buffer->size));

	return CX_ERROR_none;
}

void cx_gfx_program_param_buffer_destroy(struct cx_gfx_program_param_buffer* p_buffer) {
	struct cx_gfx_program_param_buffer_gl_internals* p_buffer_internals = (void*)p_buffer->bytes_;
	
	glDeleteBuffers(1, &p_buffer_internals->id);

	*p_buffer = (struct cx_gfx_program_param_buffer){0};
}

void cx_gfx_program_param_buffer_bind(const struct cx_gfx_program_param_buffer* p_buffer, unsigned int index) {
	const struct cx_gfx_program_param_buffer_gl_internals* p_buffer_internals = (const void*)p_buffer->bytes_;
	glBindBufferBase(GL_UNIFORM_BUFFER, index, p_buffer_internals->id);
}

void cx_gfx_program_param_buffer_bind_range(
	const struct cx_gfx_program_param_buffer* p_buffer,
	unsigned int index,
	size_t offset,
	size_t size) {
	
	const struct cx_gfx_program_param_buffer_gl_internals* p_buffer_internals = (const void*)p_buffer->bytes_;
	glBindBufferRange(GL_UNIFORM_BUFFER, index, p_buffer_internals->id, (GLsizeiptr)offset, (GLsizeiptr)size);
}

void cx_gfx_program_param_buffer_set(
	const struct cx_gfx_program_param_buffer* p_buffer,
	size_t offset,
	size_t size,
	const void* p_data) {
	
	const struct cx_gfx_program_param_buffer_gl_internals* p_buffer_internals = (const void*)p_buffer->bytes_;
	glBindBuffer(GL_UNIFORM_BUFFER, p_buffer_internals->id);
	glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)offset, size ? (GLsizeiptr)size : (GLsizeiptr)p_buffer->size, p_data);
}

void cx_gfx_program_opaque_param_bind_resource(const struct cx_gfx_program_opaque_param_binding* p_binding) {
	switch (p_binding->p_param->type) {
		case CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_sampler_2d: {
			const struct cx_gfx_texture* p_texture = p_binding->p_resource;
			const struct cx_gfx_texture_gl_internals* p_texture_internals = (const void*)p_texture->bytes_;
			glActiveTexture(GL_TEXTURE0 + p_binding->p_param->slot);
			glBindTexture(GL_TEXTURE_2D, p_texture_internals->id);
			break;
		}

		case CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_sampler_cube: {
			// todo
			break;
		}

		case CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_unknown: {
			CX_DBG(CX_LOG(ERROR, GFX_PROGRAM, "Trying to bind resource to parameter of unknown type\n"));
			return;
		}
	}
}

void cx_gfx_program_param_block_bind_buffer(const struct cx_gfx_program_param_block_binding* p_binding) {
	const struct cx_gfx_program_param_block_gl_internals* p_internals = (const void*)p_binding->p_block->bytes_;
	cx_gfx_program_param_buffer_bind_range(
		p_binding->p_buffer,
		(unsigned int)p_internals->bind_index,
		p_binding->offset,
		p_binding->size ? p_binding->size : p_binding->p_buffer->size);
}

enum cx_error cx_gfx_program_create(struct cx_gfx_program* p_program) {
	struct cx_gfx_program_gl_internals* p_program_internals = (void*)p_program->bytes_;
	p_program_internals->id = glCreateProgram();
	if (p_program_internals->id == 0) {
		return CX_ERROR_allocation_failed;
	}
	return CX_ERROR_none;
}

void cx_gfx_program_destroy(struct cx_gfx_program* p_program) {
	struct cx_gfx_program_gl_internals* p_program_internals = (void*)p_program->bytes_;
	glDeleteProgram(p_program_internals->id);
	*p_program = (struct cx_gfx_program){0};
}

int cx_gfx_program_is_built(struct cx_gfx_program* p_program) {
	const struct cx_gfx_program_gl_internals* p_internals = (const void*)p_program->bytes_;

	if (p_internals->id == 0) {
		return 0;
	}

	GLint param;

	glGetProgramiv(p_internals->id, GL_LINK_STATUS, &param);

	if (param == GL_FALSE) {
		return 0;
	}

	glValidateProgram(p_internals->id);

	glGetProgramiv(p_internals->id, GL_VALIDATE_STATUS, &param);

	return param == GL_TRUE;
}

enum cx_error cx_gfx_program_build(struct cx_gfx_program* p_program, const struct cx_gfx_program_source* p_source) {
	struct cx_gfx_program_gl_internals* p_program_internals = (void*)p_program->bytes_;

	if (p_program_internals->id == 0) {
		return CX_ERROR_invalid_state;
	}

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	if (vertex_shader == 0 || fragment_shader == 0) {
		return CX_ERROR_allocation_failed;
	}

	enum cx_error err;

	err = compile_shader_source(vertex_shader, p_source->s_vertex_stage_source);
	
	if (err != CX_ERROR_none) {
		goto end;
	}

	err = compile_shader_source(fragment_shader, p_source->s_fragment_stage_source);

	if (err != CX_ERROR_none) {
		goto end;
	}

	glAttachShader(p_program_internals->id, vertex_shader);
	glAttachShader(p_program_internals->id, fragment_shader);

	GLint param;
	
	glLinkProgram(p_program_internals->id);
	glGetProgramiv(p_program_internals->id, GL_LINK_STATUS, &param);

	if (param == GL_FALSE) {
		log_program_info_log(p_program_internals->id, CX_LOG_LEVEL_ERROR, "Program linking failed");
		err = CX_ERROR_gfx_program_build_failure;
		goto end;
	}

	glValidateProgram(p_program_internals->id);
	glGetProgramiv(p_program_internals->id, GL_VALIDATE_STATUS, &param);

	if (param == GL_FALSE) {
		log_program_info_log(p_program_internals->id, CX_LOG_LEVEL_ERROR, "Program validation failed");
		err = CX_ERROR_gfx_program_build_failure;
		goto end;
	}

	glUseProgram(p_program_internals->id);

	CX_LOG_FMT(TRACE, GFX_PROGRAM, "Program compiled: gl_id=%u\n", p_program_internals->id);

	GLint count;
	char string_buf[UNIFORM_NAME_MAX_LEN];

	CX_DBG(
		glGetProgramiv(p_program_internals->id, GL_ACTIVE_ATTRIBUTES, &count);
		
		CX_LOG_FMT(TRACE, GFX_PROGRAM, "Active input attributes: %d\n", count);

		for (GLuint i = 0; i < (GLuint)count; ++i) {
			GLint  attrib_size;
			GLenum attrib_type;
			glGetActiveAttrib(
				p_program_internals->id,
				i,
				sizeof(string_buf),
				0,
				&attrib_size,
				&attrib_type,
				string_buf);

			CX_LOG_FMT(TRACE, GFX_PROGRAM,
				"  %u: name=%s, size=%d, type=%d\n",
				i, string_buf, attrib_size, attrib_type);
		}
	);

	glGetProgramiv(p_program_internals->id, GL_ACTIVE_UNIFORMS, &count);

	CX_DBG(CX_LOG_FMT(TRACE, GFX_PROGRAM, "Active params: %d\n", count));

	GLint texture_unit = 0;

	for (GLint i = 0; i < count; ++i) {
		GLint  uniform_size;
		GLenum uniform_type;
		glGetActiveUniform(
			p_program_internals->id,
			(GLuint)i,
			UNIFORM_NAME_MAX_LEN,
			0,
			&uniform_size,
			&uniform_type,
			string_buf);

		CX_DBG(CX_LOG_FMT(TRACE, GFX_PROGRAM,
			"  %d: name='%s', size=%d, type=%d\n",
			i, string_buf, uniform_size, uniform_type));

		switch (uniform_type) {
			case GL_SAMPLER_2D: {
				glUniform1i(glGetUniformLocation(p_program_internals->id, string_buf), texture_unit);
				CX_DBG(CX_LOG_FMT(TRACE, GFX_PROGRAM, "    slot=%d\n", texture_unit));
				++texture_unit;
				break;
			}
		}
	}

	glGetProgramiv(p_program_internals->id, GL_ACTIVE_UNIFORM_BLOCKS, &count);

	CX_DBG(CX_LOG_FMT(TRACE, GFX_PROGRAM, "Active param blocks: %d\n", count));
	
	for (GLuint i = 0; i < (GLuint)count; ++i) {
		glUniformBlockBinding(p_program_internals->id, i, i);
		
		CX_DBG(
			GLint ublock_size;
			GLint ublock_active_uniforms;
			GLint ublock_active_uniform_indices[UNIFORM_BLOCK_MAX_UNIFORMS];
			GLint ublock_bind_point;

			glGetActiveUniformBlockName(p_program_internals->id, i, UNIFORM_NAME_MAX_LEN, 0, string_buf);
			
			glGetActiveUniformBlockiv(p_program_internals->id, i, GL_UNIFORM_BLOCK_DATA_SIZE, &ublock_size);
			
			glGetActiveUniformBlockiv(
				p_program_internals->id,
				i,
				GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS,
				&ublock_active_uniforms);
			
			glGetActiveUniformBlockiv(
				p_program_internals->id,
				i,
				GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES,
				ublock_active_uniform_indices);
			
			glGetActiveUniformBlockiv(
				p_program_internals->id,
				i,
				GL_UNIFORM_BLOCK_BINDING,
				&ublock_bind_point);

			CX_LOG_FMT(TRACE, GFX_PROGRAM,
				"  %d: name='%s', size=%d, bind_point=%d, active_uniforms=%d\n",
				i, string_buf, ublock_size, ublock_bind_point, ublock_active_uniforms);

			for (GLint j = 0; j < ublock_active_uniforms; ++j) {
				CX_LOG_FMT(TRACE, GFX_PROGRAM, "    %d: index=%d\n", j, ublock_active_uniform_indices[j]);
			}
		);
	}

end:
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return err;
}

int cx_gfx_program_refl_opaque_param(
	const struct cx_gfx_program* p_program,
	const char* s_name,
	struct cx_gfx_program_opaque_param* p_out_opaque_param) {

	const struct cx_gfx_program_gl_internals* p_program_internals = (const void*)p_program->bytes_;
	
	const GLint uniform_location = glGetUniformLocation(p_program_internals->id, s_name);

	if (uniform_location == -1) {
		CX_LOG_FMT(WARNING, GFX_PROGRAM, "Could not find program parameter '%s'\n", s_name);
		*p_out_opaque_param = (struct cx_gfx_program_opaque_param){0};
		return 0;
	}

	const GLint uniform_index = get_uniform_index(p_program_internals->id, s_name);

	GLsizei uniform_name_len;
	GLint   uniform_size;
	GLenum  uniform_type;
	glGetActiveUniform(
		p_program_internals->id,
		(GLuint)uniform_index,
		0,
		&uniform_name_len,
		&uniform_size,
		&uniform_type,
		0);

	switch (uniform_type) {
		case GL_SAMPLER_2D: {
			p_out_opaque_param->type = CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_sampler_2d;
			break;
		}

		case GL_SAMPLER_CUBE: {
			p_out_opaque_param->type = CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_sampler_cube;
			break;
		}

		default: {
			p_out_opaque_param->type = CX_GFX_PROGRAM_OPAQUE_PARAM_TYPE_unknown;
			break;
		}
	}

	p_out_opaque_param->n = (size_t)uniform_size;
	
	GLint opaque_slot;
	glGetUniformiv(p_program_internals->id, uniform_location, &opaque_slot);

	p_out_opaque_param->slot = (GLuint)opaque_slot;

	return 1;
}

int cx_gfx_program_refl_param_block(
	const struct cx_gfx_program* p_program,
	const char* s_name,
	struct cx_gfx_program_param_block* p_out_param_block) {

	const struct cx_gfx_program_gl_internals* p_program_internals = (const void*)p_program->bytes_;
	struct cx_gfx_program_param_block_gl_internals* p_out_blk_internals = (void*)p_out_param_block->bytes_;

	const GLuint index = glGetUniformBlockIndex(p_program_internals->id, s_name);

	if (index == GL_INVALID_INDEX) {
		CX_LOG_FMT(WARNING, GFX_PROGRAM, "Could not find program parameter block '%s'\n", s_name);
		*p_out_param_block = (struct cx_gfx_program_param_block){0};
		return 0;
	}

	glGetActiveUniformBlockiv(
		p_program_internals->id,
		index,
		GL_UNIFORM_BLOCK_BINDING,
		&p_out_blk_internals->bind_index);

	GLint block_data_size;
	glGetActiveUniformBlockiv(p_program_internals->id, index, GL_UNIFORM_BLOCK_DATA_SIZE, &block_data_size);

	p_out_param_block->size_ = (size_t)block_data_size;

	return 1;
}

void cx_gfx_program_bind(const struct cx_gfx_program* p_program) {
	static GLuint bound_program = 0;

	const struct cx_gfx_program_gl_internals* p_program_internals = (const void*)p_program->bytes_;

	if (p_program_internals->id != bound_program) {
		glUseProgram(p_program_internals->id);
		bound_program = p_program_internals->id;
	}
}

enum cx_error compile_shader_source(GLuint shader, const char* s_source) {
	glShaderSource(shader, 1, &s_source, NULL);

	glCompileShader(shader);

	GLint b_is_compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &b_is_compiled);

	if (b_is_compiled) {
		return CX_ERROR_none;
	}

	char log[1024];
	glGetShaderInfoLog(shader, sizeof(log), 0, log);

	CX_LOG_FMT(ERROR, GFX_PROGRAM, "Shader compilation failed. Reason: %s\n", log);

	return CX_ERROR_gfx_program_build_failure;
}

void log_program_info_log(GLuint program, int level, const char* s_message) {
	char info_log_buf[1024];
	glGetProgramInfoLog(program, sizeof(info_log_buf), 0, info_log_buf);
	cx_log_fmt(
		level,
		CX_LOG_CAT_GFX_PROGRAM,
		"%s: id=%d, info_log=\"%s\"\n",
		s_message, program, info_log_buf);
}

GLint get_uniform_index(GLuint program, const char* s_name) {
	GLint count;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

	char string_buf[UNIFORM_NAME_MAX_LEN];

	for (GLint i = 0; i < count; ++i) {
		GLint  uniform_size;
		GLenum uniform_type;
		glGetActiveUniform(
			program,
			(GLuint)i,
			UNIFORM_NAME_MAX_LEN,
			0,
			&uniform_size,
			&uniform_type,
			string_buf);
		if (strcmp(s_name, string_buf) == 0) {
			return i;
		}
	}

	return -1; 
}
