#ifndef _H__CX_GFX_CONTEXT
#define _H__CX_GFX_CONTEXT

#include "errors.h"

#define CX_LOG_CAT_GFX_CORE "gfx:core"

struct platform_window;

struct cx_gfx_context {
    char _bytes[24];
};

enum error cx_gfx_context_create(const struct platform_window* p_window, struct cx_gfx_context* p_out_context);
void       cx_gfx_context_destroy(struct cx_gfx_context* p_context);
enum error cx_gfx_context_make_current(const struct cx_gfx_context* p_context);
enum error cx_gfx_context_swap_buffers(const struct cx_gfx_context* p_context);

#endif
