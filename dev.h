#ifndef _H__DEV
#define _H__DEV

#include <stdint.h>

#include "gl.h"

struct platform_window;
struct scene;
struct physics_world;

void dev_init(const struct platform_window* p_platform_window, struct scene* p_scene, struct physics_world* p_physics_world);
void dev_shutdown(void);
void dev_draw(GLuint gl_framebuffer, size_t framebuffer_width, size_t framebuffer_height, const float* p_projection_matrix, const float* p_view_matrix);

#endif