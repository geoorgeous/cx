#ifndef SKELETAL_ANIMATION_DEBUG_H
#define SKELETAL_ANIMATION_DEBUG_H

struct skeleton;

void debug_draw_skeleton(const struct skeleton* p_skeleton, const float* p_projection_matrix, const float* p_view_matrix, const float* p_model_matrix);

#endif
