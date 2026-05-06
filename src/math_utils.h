#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <float.h>
#define FLT_CMP(A, B) (fabsf(A - B) < FLT_EPSILON)
#define FLT_ISZERO(A) (fabsf(A) < FLT_EPSILON)

int ray_plane_intersection(const float* p_ray_origin, const float* p_ray_dir, const float* p_plane_norm, float plane_offset, float* p_result);

void compute_compliment_axes(const float* p_forward, float* p_side, float* p_up);

float randf(void);

float segment_point_dist_sq(const float* p_p0, const float* p_p1, const float* p_q);

int signf(float x);

float clampf(const float x, const float min, const float max);

#endif
