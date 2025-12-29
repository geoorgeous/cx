#ifndef _H__DEV_DRAW
#define _H__DEV_DRAW

#include "cx_color.h"

void dev_draw_line(const float* p_p0, const float* p_p1, u32_r8g8b8a8 p_color, float duration);
void dev_draw_sphere(const float* p_center, float radius, u32_r8g8b8a8 p_line_color, u32_r8g8b8a8 p_fill_color, float duration);
void dev_draw_capsule(const float* p_p0, const float* p_p1, float radius, u32_r8g8b8a8 p_line_color, u32_r8g8b8a8 p_fill_color, float duration);
void dev_draw_box(const float* p_min, const float* p_max, u32_r8g8b8a8 p_line_color, u32_r8g8b8a8 p_fill_color, float duration);
void dev_draw_plane(const float* p_normal, float distance, u32_r8g8b8a8 p_line_color, u32_r8g8b8a8 p_fill_color, float duration);

#endif