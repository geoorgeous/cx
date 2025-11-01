#ifndef _H__MATH_UTILS
#define _H__MATH_UTILS

#include <float.h>
#define FLT_CMP(A, B) (fabsf(A - B) < FLT_EPSILON)

int ray_plane_intersection(const float* p_ray_origin, const float* p_ray_dir, const float* p_plane_norm, float plane_offset, float* p_result);

void compute_compliment_axes(const float* p_forward, float* p_side, float* p_up);

float randf(void);

float segment_point_dist_sq(const float* p_p0, const float* p_p1, const float* p_q);

int signf(float x);

#endif