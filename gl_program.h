#ifndef _H__GL_PROGRAM
#define _H__GL_PROGRAM

#include "errors.h"
#include "gl.h"

struct gl_shader {
    GLuint gl_handle;
};

enum error gl_shader_create(struct gl_shader* p_gl_shader, GLenum gl_shader_type);
enum error gl_shader_compile(struct gl_shader* p_gl_shader, const char* s_source);
void       gl_shader_destroy(struct gl_shader* p_gl_shader);

enum shader_uniform_type {
    SHADER_UNIFORM_TYPE_vec2,
    SHADER_UNIFORM_TYPE_vec3,
    SHADER_UNIFORM_TYPE_vec4,
    SHADER_UNIFORM_TYPE_mat2,
    SHADER_UNIFORM_TYPE_mat3,
    SHADER_UNIFORM_TYPE_mat4,
};

struct gl_program {
    GLuint gl_handle;
};

enum error gl_program_create(struct gl_program* p_gl_program);
enum error gl_program_attach_shader(struct gl_program* p_gl_program, const struct gl_shader* p_gl_shader);
enum error gl_program_link(struct gl_program* p_gl_program);
void       gl_program_destroy(struct gl_program* p_gl_program);
void       gl_program_print_info(const struct gl_program* p_gl_program);

#endif