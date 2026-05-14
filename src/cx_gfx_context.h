#ifndef CX_GFX_CONTEXT_H
#define CX_GFX_CONTEXT_H

#include "cx_error.h"

#define CX_LOG_CAT_GFX_CORE "gfx:core"

struct platform_window;

struct cx_gfx_context {
    char bytes_[24];
};

enum cx_error cx_gfx_context_create(const struct platform_window* p_window, struct cx_gfx_context* p_out_context);

void cx_gfx_context_destroy(struct cx_gfx_context* p_context);

enum cx_error cx_gfx_context_make_current(const struct cx_gfx_context* p_context);

enum cx_error cx_gfx_context_swap_buffers(const struct cx_gfx_context* p_context);

unsigned int cx_gfx_context_get_swap_interval(const struct cx_gfx_context* p_context);

enum cx_error cx_gfx_context_set_swap_interval(const struct cx_gfx_context* p_context, unsigned int interval);

#endif
