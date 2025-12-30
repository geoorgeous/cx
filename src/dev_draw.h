#ifndef _H__DEV_DRAW
#define _H__DEV_DRAW

#include "cx_color.h"

struct gl_mesh;

void dev_draw_mesh(const struct gl_mesh* p_gl_mesh, const float* p_transform, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration);
void dev_draw_sphere(const float* p_center, float radius, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration);
void dev_draw_capsule(const float* p_p0, const float* p_p1, float radius, u32_r8g8b8a8 line_color, u32_r8g8b8a8 p_fill_color, float duration);
void dev_draw_box(const float* p_min, const float* p_max, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration);
void dev_draw_plane(const float* p_normal, float distance, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration);
void dev_draw_line(const float* p_p0, const float* p_p1, u32_r8g8b8a8 color, float duration);

void dev_draw_flush(const float* p_projection_matrix, const float* p_view_matrix, float delta_time);

#endif