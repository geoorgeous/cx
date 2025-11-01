#ifndef _H__GL_CONTEXT
#define _H__GL_CONTEXT

#include "errors.h"
#include "platform.h"

struct platform_window;

struct gl_context {
#ifdef PLATFORM_WINDOWS
    HDC   _hdc;
    HGLRC _hrc;
#endif
};

enum error gl_context_create(const struct platform_window* p_platform_window, int gl_version_major, int gl_version_minor, struct gl_context* p_gl_context);
void       gl_context_destroy(struct gl_context* p_gl_context);
enum error gl_context_make_current(const struct gl_context* p_gl_context);
enum error gl_context_swap_buffers(struct gl_context* p_gl_context);

#endif