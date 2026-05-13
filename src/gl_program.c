#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gl_program.h"
#include "cx_logging.h"
#include "cx_error.h"

static void set_uniform_int(GLint gl_location, size_t count, const int* p_data);
static void set_uniform_uint(GLint gl_location, size_t count, const unsigned int* p_data);
static void set_uniform_float(GLint gl_location, size_t count, const float* p_data);
static void set_uniform_vec2(GLint gl_location, size_t count, const float* p_data);
static void set_uniform_vec3(GLint gl_location, size_t count, const float* p_data);
static void set_uniform_vec4(GLint gl_location, size_t count, const float* p_data);
static void set_uniform_mat2(GLint gl_location, size_t count, const float* p_data);
static void set_uniform_mat3(GLint gl_location, size_t count, const float* p_data);
static void set_uniform_mat4(GLint gl_location, size_t count, const float* p_data);

enum cx_error gl_shader_create(struct gl_shader* p_gl_shader, GLenum gl_shader_type) {
	p_gl_shader->gl_handle = glCreateShader(gl_shader_type);
	
	if (p_gl_shader->gl_handle) {
		return CX_ERROR_none;
	}

	return CX_ERROR_gfx_program_build_failure;
}

enum cx_error gl_shader_compile(struct gl_shader* p_gl_shader, const char* s_source) {
	if (!glIsShader(p_gl_shader->gl_handle)) {
		return CX_ERROR_invalid_argument;
	}

	glShaderSource(p_gl_shader->gl_handle, 1, &s_source, NULL);

	glCompileShader(p_gl_shader->gl_handle);

	GLint b_is_compiled;
	glGetShaderiv(p_gl_shader->gl_handle, GL_COMPILE_STATUS, &b_is_compiled);

	if (b_is_compiled) {
		return CX_ERROR_none;
	}

	GLint log_len = 0;
	glGetShaderiv(p_gl_shader->gl_handle, GL_INFO_LOG_LENGTH, &log_len);

	char* s_log = malloc(log_len);
	glGetShaderInfoLog(p_gl_shader->gl_handle, log_len, &log_len, s_log);

	CX_LOG_FMT(ERROR, DONTCARE, "Shader compilation failed: %s\n", s_log);

	free(s_log);

	return CX_ERROR_gfx_program_build_failure;
}

void gl_shader_destroy(struct gl_shader* p_gl_shader) {
	glDeleteShader(p_gl_shader->gl_handle);
}

enum cx_error gl_program_create(struct gl_program* p_gl_program) {
	p_gl_program->gl_handle = glCreateProgram();

	if (p_gl_program->gl_handle) {
		return CX_ERROR_none;
	}

	return CX_ERROR_allocation_failed;
}

enum cx_error gl_program_attach_shader(struct gl_program* p_gl_program, const struct gl_shader* p_gl_shader) {
	if (!glIsProgram(p_gl_program->gl_handle)) {
		return CX_ERROR_invalid_argument;
	}

	if (!glIsShader(p_gl_shader->gl_handle)) {
		return CX_ERROR_invalid_argument;
	}

	glAttachShader(p_gl_program->gl_handle, p_gl_shader->gl_handle);

	return CX_ERROR_none;
}

enum cx_error gl_program_link(struct gl_program* p_gl_program) {
	if (!glIsProgram(p_gl_program->gl_handle)) {
		return CX_ERROR_invalid_argument;
	}

	glLinkProgram(p_gl_program->gl_handle);

	GLint b_is_linked;
	glGetProgramiv(p_gl_program->gl_handle, GL_LINK_STATUS, &b_is_linked);
	
	if (b_is_linked) {
		return CX_ERROR_none;
	}

	return CX_ERROR_gfx_program_build_failure;
}

void gl_program_destroy(struct gl_program* p_gl_program) {
	glDeleteProgram(p_gl_program->gl_handle);
}

void gl_program_print_info(const struct gl_program* p_gl_program) {
	GLint result;

	glGetProgramiv(p_gl_program->gl_handle, GL_ACTIVE_UNIFORMS, &result);

	for (GLint i = 0; i < result; ++i) {
		GLsizei uniform_name_len;
		GLint uniform_size;  
		GLenum uniform_type;
		GLchar uniform_name[64];
		glGetActiveUniform(p_gl_program->gl_handle, i, sizeof(uniform_name), &uniform_name_len, &uniform_size, &uniform_type, uniform_name);
	}
}

void gl_program_get_uniform(const struct gl_program* p_gl_program, const char* s_uniform_name, struct gl_program_uniform* p_uniform) {
	p_uniform->_gl_location = glGetUniformLocation(p_gl_program->gl_handle, s_uniform_name);

	if (p_uniform->_gl_location == -1) {
		CX_LOG_FMT(WARNING, DONTCARE, "Program uniform \"%s\" not found\n", s_uniform_name);
		*p_uniform = (struct gl_program_uniform){0};
		return;
	}

	GLint num_active_uniforms;
	glGetProgramiv(p_gl_program->gl_handle, GL_ACTIVE_UNIFORMS, &num_active_uniforms);

	for (GLint i = 0; i < num_active_uniforms; ++i) {
		GLsizei uniform_name_len;
		GLint uniform_size;  
		GLenum uniform_type;
		GLchar uniform_name[64];
		glGetActiveUniform(p_gl_program->gl_handle, i, sizeof(uniform_name), &uniform_name_len, &uniform_size, &uniform_type, uniform_name);

		if (strncmp(s_uniform_name, uniform_name, uniform_name_len)) {
			continue;
		}

		switch(uniform_type) {
			default:              p_uniform->_type = GL_SHADER_UNIFORM_TYPE_none; break;
			case GL_INT:          p_uniform->_type = GL_SHADER_UNIFORM_TYPE_int; break;
			case GL_UNSIGNED_INT: p_uniform->_type = GL_SHADER_UNIFORM_TYPE_uint; break;
			case GL_FLOAT:        p_uniform->_type = GL_SHADER_UNIFORM_TYPE_float; break;
			case GL_FLOAT_VEC2:   p_uniform->_type = GL_SHADER_UNIFORM_TYPE_vec2; break;
			case GL_FLOAT_VEC3:   p_uniform->_type = GL_SHADER_UNIFORM_TYPE_vec3; break;
			case GL_FLOAT_VEC4:   p_uniform->_type = GL_SHADER_UNIFORM_TYPE_vec4; break;
			case GL_FLOAT_MAT2:   p_uniform->_type = GL_SHADER_UNIFORM_TYPE_mat2; break;
			case GL_FLOAT_MAT3:   p_uniform->_type = GL_SHADER_UNIFORM_TYPE_mat3; break;
			case GL_FLOAT_MAT4:   p_uniform->_type = GL_SHADER_UNIFORM_TYPE_mat4; break;
		}

		break;
	}
}

typedef void(*set_uniform_func)(GLuint, size_t, const void*);

void gl_program_uniform_set(const struct gl_program_uniform* p_uniform, size_t count, const void* p_data) {
	static const set_uniform_func func_table[] = {
		0,
		(void*)set_uniform_int,
		(void*)set_uniform_uint,
		(void*)set_uniform_float,
		(void*)set_uniform_vec2,
		(void*)set_uniform_vec3,
		(void*)set_uniform_vec4,
		(void*)set_uniform_mat2,
		(void*)set_uniform_mat3,
		(void*)set_uniform_mat4
	};

	if (p_uniform->_type == GL_SHADER_UNIFORM_TYPE_none) {
		CX_LOG(ERROR, DONTCARE, "Cannot set uniforms of no type.\n");
		return;
	}

	func_table[p_uniform->_type](p_uniform->_gl_location, count, p_data);
}

void set_uniform_int(GLint gl_location, size_t count, const int* p_data) {
	glUniform1iv(gl_location, (GLsizei)count, (const GLint*)p_data);
}

void set_uniform_uint(GLint gl_location, size_t count, const unsigned int* p_data) {
	glUniform1uiv(gl_location, (GLsizei)count, (const GLuint*)p_data);
}

void set_uniform_float(GLint gl_location, size_t count, const float* p_data) {
	glUniform1fv(gl_location, (GLsizei)count, (const GLfloat*)p_data);
}

void set_uniform_vec2(GLint gl_location, size_t count, const float* p_data) {
	glUniform2fv(gl_location, (GLsizei)count, (const GLfloat*)p_data);
}

void set_uniform_vec3(GLint gl_location, size_t count, const float* p_data) {
	glUniform3fv(gl_location, (GLsizei)count, (const GLfloat*)p_data);
}

void set_uniform_vec4(GLint gl_location, size_t count, const float* p_data) {
	glUniform4fv(gl_location, (GLsizei)count, (const GLfloat*)p_data);
}

void set_uniform_mat2(GLint gl_location, size_t count, const float* p_data) {
	glUniformMatrix2fv(gl_location, (GLsizei)count, GL_FALSE, (const GLfloat*)p_data);
}

void set_uniform_mat3(GLint gl_location, size_t count, const float* p_data) {
	glUniformMatrix3fv(gl_location, (GLsizei)count, GL_FALSE, (const GLfloat*)p_data);
}

void set_uniform_mat4(GLint gl_location, size_t count, const float* p_data) {
	glUniformMatrix4fv(gl_location, (GLsizei)count, GL_FALSE, (const GLfloat*)p_data);
}
