#ifndef CX_GFX_SHADER_PROGRAM_INTERFACE_GL_H
#define CX_GFX_SHADER_PROGRAM_INTERFACE_GL_H

#include "gl.h"

struct cx_gfx_shader_program_parameter_info_gl_internals {
	GLint uniform_location;
};

struct cx_gfx_shader_program_texture_info_gl_internals {
	GLenum texture_target;
	GLint texture_unit;
};

struct cx_gfx_shader_program_block_info_gl_internals {
	GLuint uniform_block_binding_point;
};

#endif
