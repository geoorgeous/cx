#include "cx_gfx_program.h"
#include "cx_dbg.h"
#include "gl.h"
#include "cx_logging.h"

#define UNIFORM_NAME_MAX_LEN 64
#define UNIFORM_BLOCK_MAX_UNIFORMS 64

struct cx_gfx_program_param_gl_internals {
	GLint uniform_location;
};

struct cx_gfx_program_gl_internals {
	GLuint id;
};

static cx_result compile_shader_source(GLuint shader, const char* s_source);
void             log_program_info_log(GLuint program, int level, const char* message);

cx_result cx_gfx_program_create(struct cx_gfx_program* p_program) {
	struct cx_gfx_program_gl_internals* p_program_internals = (void*)p_program->internals_.bytes_;
	
	(void)glGetError();
	p_program_internals->id = glCreateProgram();
	
	if (p_program_internals->id == 0) {
		GLenum glerr = glGetError();
		return
			glerr == GL_INVALID_OPERATION ? CX_ERROR_INVALID_OPERATION :
			glerr == GL_OUT_OF_MEMORY ? CX_ERROR_OUT_OF_MEMORY :
			CX_ERROR_UNKNOWN;
	}
	return CX_SUCCESS;
}

void cx_gfx_program_destroy(struct cx_gfx_program* p_program) {
	struct cx_gfx_program_gl_internals* p_program_internals = (void*)p_program->internals_.bytes_;
	glDeleteProgram(p_program_internals->id);
	*p_program = (struct cx_gfx_program){0};
}

int cx_gfx_program_is_built(struct cx_gfx_program* p_program) {
	const struct cx_gfx_program_gl_internals* p_internals = (const void*)p_program->internals_.bytes_;

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

cx_result cx_gfx_program_build(struct cx_gfx_program* p_program, const struct cx_gfx_program_source* p_source) {
	struct cx_gfx_program_gl_internals* p_program_internals = (void*)p_program->internals_.bytes_;

	if (p_program_internals->id == 0) {
		return CX_ERROR_INVALID_ARG;
	}

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	if (vertex_shader == 0 || fragment_shader == 0) {
		return CX_ERROR_UNKNOWN;
	}

	cx_result result;

	result = compile_shader_source(vertex_shader, p_source->s_vertex_stage_source);
	if (result != CX_SUCCESS) {
		goto end;
	}

	result = compile_shader_source(fragment_shader, p_source->s_fragment_stage_source);
	if (result != CX_SUCCESS) {
		goto end;
	}

	glAttachShader(p_program_internals->id, vertex_shader);
	glAttachShader(p_program_internals->id, fragment_shader);

	GLint param;
	
	glLinkProgram(p_program_internals->id);
	glGetProgramiv(p_program_internals->id, GL_LINK_STATUS, &param);

	if (param == GL_FALSE) {
		log_program_info_log(p_program_internals->id, CX_LOG_LEVEL_ERROR, "Program linking failed");
		result = CX_ERROR_INVALID_ARG;
		goto end;
	}

	glValidateProgram(p_program_internals->id);
	glGetProgramiv(p_program_internals->id, GL_VALIDATE_STATUS, &param);

	if (param == GL_FALSE) {
		log_program_info_log(p_program_internals->id, CX_LOG_LEVEL_ERROR, "Program validation failed");
		result = CX_ERROR_INVALID_ARG;
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

	CX_ASSERT(count <= CX_GFX_PROGRAM_MAX_PARAMS, GFX_PROGRAM);

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

		// todo GL enum -> cx param type
		p_program->params[i].type = 0;
		
		struct cx_gfx_program_param_gl_internals* p_program_param_internals =
			(void*)&p_program->params[i].u.internals_.bytes_;

		p_program_param_internals->uniform_location = glGetUniformLocation(p_program_internals->id, string_buf);

		p_program->num_params++;

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

	CX_ASSERT(count <= CX_GFX_PROGRAM_MAX_PARAM_BLOCKS, GFX_PROGRAM);
	CX_ASSERT(p_program->num_params + count <= CX_GFX_PROGRAM_MAX_PARAMS, GFX_PROGRAM);

	CX_DBG(CX_LOG_FMT(TRACE, GFX_PROGRAM, "Active param blocks: %d\n", count));
	
	for (GLuint i = 0; i < (GLuint)count; ++i) {
		glUniformBlockBinding(p_program_internals->id, i, i);
		
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

		p_program->params[p_program->num_params + i].type = CX_GFX_PROGRAM_PARAM_TYPE_block;
		p_program->params[p_program->num_params + i].u.block_index = (uint16_t)i;
		p_program->num_params++;

		p_program->param_blocks[i].size = (size_t)ublock_size;
		// todo: internals?
		p_program->num_param_blocks++;

		CX_DBG(
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

	return result;
}

void cx_gfx_program_bind(const struct cx_gfx_program* p_program) {
	static GLuint bound_program = 0;

	const struct cx_gfx_program_gl_internals* p_program_internals = (const void*)p_program->internals_.bytes_;

	if (p_program_internals->id != bound_program) {
		glUseProgram(p_program_internals->id);
		bound_program = p_program_internals->id;
	}
}

cx_result compile_shader_source(GLuint shader, const char* s_source) {
	glShaderSource(shader, 1, &s_source, NULL);

	glCompileShader(shader);

	GLint b_is_compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &b_is_compiled);

	if (b_is_compiled) {
		return CX_SUCCESS;
	}

	char log[1024];
	glGetShaderInfoLog(shader, sizeof(log), 0, log);

	CX_LOG_FMT(ERROR, GFX_PROGRAM, "Shader compilation failed. Reason: %s\n", log);

	return CX_ERROR_INVALID_ARG;
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
