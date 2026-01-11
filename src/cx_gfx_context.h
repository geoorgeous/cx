#ifndef _H__GL_CONTEXT
#define _H__GL_CONTEXT

#include "errors.h"

#define CX_LOG_CAT_GFX_CORE "gfx:core"

struct platform_window;

struct gl_context {
    char _bytes[24];
};

enum error gl_context_create(int gl_version_major, int gl_version_minor, const struct platform_window* p_window, struct gl_context* p_out_context);
void       gl_context_destroy(struct gl_context* p_context);
enum error gl_context_make_current(const struct gl_context* p_context);
enum error gl_context_swap_buffers(const struct gl_context* p_context);

#endif
