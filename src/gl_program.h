#ifndef GL_PROGRAM_H
#define GL_PROGRAM_H

#include "errors.h"
#include "gl.h"

struct gl_shader {
	GLuint gl_handle;
};

enum error gl_shader_create(struct gl_shader* p_gl_shader, GLenum gl_shader_type);
enum error gl_shader_compile(struct gl_shader* p_gl_shader, const char* s_source);
void       gl_shader_destroy(struct gl_shader* p_gl_shader);

enum gl_shader_uniform_type {
	GL_SHADER_UNIFORM_TYPE_none,
	GL_SHADER_UNIFORM_TYPE_int,
	GL_SHADER_UNIFORM_TYPE_uint,
	GL_SHADER_UNIFORM_TYPE_float,
	GL_SHADER_UNIFORM_TYPE_vec2,
	GL_SHADER_UNIFORM_TYPE_vec3,
	GL_SHADER_UNIFORM_TYPE_vec4,
	GL_SHADER_UNIFORM_TYPE_mat2,
	GL_SHADER_UNIFORM_TYPE_mat3,
	GL_SHADER_UNIFORM_TYPE_mat4
};

struct gl_program {
	GLuint gl_handle;
};

struct gl_program_uniform;

enum error gl_program_create(struct gl_program* p_gl_program);
enum error gl_program_attach_shader(struct gl_program* p_gl_program, const struct gl_shader* p_gl_shader);
enum error gl_program_link(struct gl_program* p_gl_program);
void       gl_program_destroy(struct gl_program* p_gl_program);
void       gl_program_print_info(const struct gl_program* p_gl_program);
void       gl_program_get_uniform(const struct gl_program* p_gl_program, const char* s_uniform_name, struct gl_program_uniform* p_uniform);

struct gl_program_uniform {
	enum gl_shader_uniform_type _type;
	GLint                       _gl_location;
};

void gl_program_uniform_set(const struct gl_program_uniform* p_uniform, size_t count, const void* p_data);

#endif
