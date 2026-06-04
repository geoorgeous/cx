#include "gl.h"

struct cx_gfx_buffer_gl_internals {
	GLuint id;
	GLenum target;
	GLenum usage;
};

