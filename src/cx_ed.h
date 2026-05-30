#ifndef CX_ED_H
#define CX_ED_H

#include <stdint.h>

#define CX_LOG_CAT_ED "editor"

// action history
// entity names
// selected entity
// draw selected entity bounds
// draw gizmos for selected entity
// gizmo dragging, snapping
//
//
// actions:
//  - edit entity transform
//  - create entity
//  - delete entity
//  - add entity component
//  - delete entity component

struct platform_window;

void cx_ed_init(struct platform_window* p_window);

void cx_ed_shutdown(void);

void cx_ed_update(double dt_seconds);

struct cx_gfx_framebuffer;

void cx_ed_draw(const struct cx_gfx_framebuffer* p_fb, uint32_t fb_width, uint32_t fb_height);

#endif
