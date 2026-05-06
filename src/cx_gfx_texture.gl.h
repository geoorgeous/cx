#ifndef CX_GFX_TEXTURE_GL_H
#define CX_GFX_TEXTURE_GL_H

#include "gl.h"

struct cx_gfx_texture_gl_internals {
	GLuint id;
	int    b_mipmaps;
};

#endif
