#ifndef DEV_H
#define DEV_H

#include <stdint.h>

#include "gl.h"

#define CX_LOG_CAT_DEV "dev"

#define ENSURE_DEV_MODE() if (!dev_mode_is_enabled()) { return; }

void dev_mode_enable(void);

void dev_mode_disable(void);

int  dev_mode_is_enabled(void);

struct platform_window;
struct scene;
struct physics_world;

void dev_init(const struct platform_window* p_window, struct scene* p_scene, struct physics_world* p_physics_world);

void dev_shutdown(void);

struct cx_gfx_framebuffer;

void dev_draw(
	const struct cx_gfx_framebuffer* p_framebuffer,
	uint32_t framebuffer_width, uint32_t framebuffer_height,
	const float* p_projection_matrix,
	const float* p_view_matrix);

#endif
