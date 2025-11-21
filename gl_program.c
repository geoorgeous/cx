#include <stdio.h>

#include "gl_program.h"
#include "logging.h"

enum error gl_shader_create(struct gl_shader* p_gl_shader, GLenum gl_shader_type) {
    p_gl_shader->gl_handle = glCreateShader(gl_shader_type);
    
    if (p_gl_shader->gl_handle) {
        return ERROR_OK;
    }

    return ERROR_OPENGL;
}

enum error gl_shader_compile(struct gl_shader* p_gl_shader, const char* s_source) {
    if (!glIsShader(p_gl_shader->gl_handle)) {
        return ERROR_INVALID_VALUE;
    }

    glShaderSource(p_gl_shader->gl_handle, 1, &s_source, NULL);

    glCompileShader(p_gl_shader->gl_handle);

    GLint b_is_compiled;
    glGetShaderiv(p_gl_shader->gl_handle, GL_COMPILE_STATUS, &b_is_compiled);

    if (b_is_compiled) {
        return ERROR_OK;
    }

	GLint log_len = 0;
	glGetShaderiv(p_gl_shader->gl_handle, GL_INFO_LOG_LENGTH, &log_len);

    char* s_log = malloc(log_len);
	glGetShaderInfoLog(p_gl_shader->gl_handle, log_len, &log_len, s_log);

    cx_log_fmt(CX_LOG_ERROR, 0, "Shader compilation failed: %s\n", s_log);

    free(s_log);

    return ERROR_SHADER_COMPILATION;
}

void gl_shader_destroy(struct gl_shader* p_gl_shader) {
    glDeleteShader(p_gl_shader->gl_handle);
}

enum error gl_program_create(struct gl_program* p_gl_program) {
    p_gl_program->gl_handle = glCreateProgram();

    if (p_gl_program->gl_handle) {
        return ERROR_OK;
    }

    return ERROR_OPENGL;
}

enum error gl_program_attach_shader(struct gl_program* p_gl_program, const struct gl_shader* p_gl_shader) {
    if (!glIsProgram(p_gl_program->gl_handle)) {
        return ERROR_INVALID_VALUE;
    }

    if (!glIsShader(p_gl_shader->gl_handle)) {
        return ERROR_INVALID_VALUE;
    }

    glAttachShader(p_gl_program->gl_handle, p_gl_shader->gl_handle);

    return ERROR_OK;
}

enum error gl_program_link(struct gl_program* p_gl_program) {
    if (!glIsProgram(p_gl_program->gl_handle)) {
        return ERROR_INVALID_VALUE;
    }

    glLinkProgram(p_gl_program->gl_handle);

    GLint b_is_linked;
    glGetProgramiv(p_gl_program->gl_handle, GL_LINK_STATUS, &b_is_linked);
    
    if (b_is_linked) {
        return ERROR_OK;
    }

    return ERROR_SHADER_PROGRAM_LINKAGE;
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