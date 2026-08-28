#ifndef CX_ED_H
#define CX_ED_H

#include <stdint.h>

#define CX_LOG_CAT_ED_WORLD_EDITOR "ed:world_editor"

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

struct cx_platform_window;
struct cx_asset_ref;

void cx_ed_world_editor_init(struct cx_platform_window* p_window, const char* s_world_blueprint_asset_name);

void cx_ed_world_editor_shutdown(void);

void cx_ed_world_editor_update(double dt_seconds);

struct cx_gfx_framebuffer;

void cx_ed_world_editor_draw(const struct cx_gfx_framebuffer* p_fb, uint32_t fb_width, uint32_t fb_height);

#endif
