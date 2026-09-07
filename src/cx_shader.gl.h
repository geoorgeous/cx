#ifndef CX_SHADER_GL_H
#define CX_SHADER_GL_H

#include "gl.h"

struct cx_shader_binding_param_gl_internals {
	GLint uniform_location;
};

struct cx_shader_binding_block_gl_internals {
	GLuint uniform_block_binding_point;
};

struct cx_shader_binding_block_member_gl_internals {
	GLint uniform_block_binding_point;
};

struct cx_shader_binding_sampler_gl_internals {
	GLenum texture_target;
	GLint texture_unit;
};

#endif
