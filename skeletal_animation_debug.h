#ifndef _H__SKELETAL_ANIMATION_DEBUG
#define _H__SKELETAL_ANIMATION_DEBUG

struct skeleton;

void debug_draw_skeleton(const struct skeleton* p_skeleton, const float* p_projection_matrix, const float* p_view_matrix, const float* p_model_matrix);

#endif