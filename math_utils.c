#include <math.h>
#include <stdlib.h>

#include "math_utils.h"
#include "vector.h"

int ray_plane_intersection(const float* p_ray_origin, const float* p_ray_dir, const float* p_plane_norm, float plane_offset, float* p_result) {
    const float denom = vec3_dot(p_plane_norm, p_ray_dir);

    if (fabsf(denom) < 0.0001f) {
        return 0;
    }

    const float t = -(vec3_dot(p_plane_norm, p_ray_origin) + plane_offset) / denom;

    if (FLT_CMP(t, 0)) {
        return 0;
    }

    vec3_mul_s(p_ray_dir, t, p_result);
    vec3_add(p_ray_origin, p_result, p_result);

    return 1;
}

void compute_compliment_axes(const float* p_forward, float* p_side, float* p_up) {
    const float world_side[] = { 0, 0, 1 };
    const float world_up[] = { 0, 1, 0 };
    const float* p_t = vec3_cmp(p_forward, world_up) ? world_side : world_up;
    vec3_cross(p_forward, p_t, p_side);
    vec3_cross(p_side, p_forward, p_up);
}

float randf(void) {
    return ((float)rand()/(float)(RAND_MAX));
}

float segment_point_dist_sq(const float* p_p0, const float* p_p1, const float* p_q) {
    float v0[3];
    float v1[3];

    vec3_sub(p_q, p_p0, v0);
    vec3_sub(p_p1, p_p0, v1);
    
    const float s0 = vec3_dot(v0, v1);
    const float s1 = vec3_dot(v1, v1);
    
    float s2 = FLT_CMP(s1, 0) ? -1 : s0 / s1;

    if (s2 < 0) {
        vec3_sub(p_q, p_p0, v0);
    } else if (s2 > 1) {
        vec3_sub(p_q, p_p1, v0);
    } else {
        vec3_mul_s(v1, s2, v1);
        vec3_add(p_p0, v1, v0);
        vec3_sub(p_q, v0, v0);
    }
    
    return vec3_dot(v0, v0);
}

int signf(float x) {
    return (x > 0) - (x < 0);
}