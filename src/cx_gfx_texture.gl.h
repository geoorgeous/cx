#ifndef _H__CX_GFX_TEXTURE_GL
#define _H__CX_GFX_TEXTURE_GL

#include "gl.h"

struct cx_gfx_texture_gl_internals {
	GLuint id;
	int    b_mipmaps;
};

#endif
