#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <stddef.h>

#include "math_utils.h"

void  vec_copy    (size_t n, const float* p_v, float* p_out);
void  vec_set     (size_t n, float s, float* p_out);
void  vec_clr     (size_t n, float* p_out);
void  vec_add     (size_t n, const float* p_v1, const float* p_v2, float* p_out);
void  vec_add_s   (size_t n, const float* p_v, float s, float* p_out);
void  vec_sub     (size_t n, const float* p_v1, const float* p_v2, float* p_out);
void  vec_sub_s   (size_t n, const float* p_v, float s, float* p_out);
void  vec_mul     (size_t n, const float* p_v1, const float* p_v2, float* p_out);
void  vec_mul_s   (size_t n, const float* p_v, float s, float* p_out);
void  vec_div     (size_t n, const float* p_v1, const float* p_v2, float* p_out);
void  vec_div_s   (size_t n, const float* p_v, float s, float* p_out);
float vec_dot     (size_t n, const float* p_v1, const float* p_v2);
float vec_len_sq  (size_t n, const float* p_v);
float vec_len     (size_t n, const float* p_v);
void  vec_norm    (size_t n, const float* p_v, float* p_out);
int   vec_cmp     (size_t n, const float* p_v1, const float* p_v2);
int   vec_is_zero (size_t n, const float* p_v);
void  vec_min     (size_t n, const float* p_v1, const float* p_v2, float* p_out);
void  vec_max     (size_t n, const float* p_v1, const float* p_v2, float* p_out);
float vec_minor   (size_t n, const float* p_v);
float vec_major   (size_t n, const float* p_v);
float vec_sum     (size_t n, const float* p_v);
void  vec_inv     (size_t n, const float* p_v, float* p_out);

inline void vec3_copy(const float* p_v, float* p_out) {
    p_out[0] = p_v[0];
    p_out[1] = p_v[1];
    p_out[2] = p_v[2];
}

inline void vec3_set_s(float s, float* p_out) {
    p_out[0] =
    p_out[1] =
    p_out[2] = s;
}

inline void vec3_set(float i, float j, float k, float* p_out) {
    p_out[0] = i;
    p_out[1] = j;
    p_out[2] = k;
}

inline void vec3_clr(float* p_out) {
    vec3_set_s(0, p_out);
}

inline void vec3_add(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] + p_v2[0];
    p_out[1] = p_v1[1] + p_v2[1];
    p_out[2] = p_v1[2] + p_v2[2];
}

inline void vec3_add_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] + s;
    p_out[1] = p_v[1] + s;
    p_out[2] = p_v[2] + s;
}

inline void vec3_sub(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] - p_v2[0];
    p_out[1] = p_v1[1] - p_v2[1];
    p_out[2] = p_v1[2] - p_v2[2];
}

inline void vec3_sub_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] - s;
    p_out[1] = p_v[1] - s;
    p_out[2] = p_v[2] - s;
}

inline void vec3_mul(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] * p_v2[0];
    p_out[1] = p_v1[1] * p_v2[1];
    p_out[2] = p_v1[2] * p_v2[2];
}

inline void vec3_mul_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] * s;
    p_out[1] = p_v[1] * s;
    p_out[2] = p_v[2] * s;
}

inline void vec3_div(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] / p_v2[0];
    p_out[1] = p_v1[1] / p_v2[1];
    p_out[2] = p_v1[2] / p_v2[2];
}

inline void vec3_div_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] / s;
    p_out[1] = p_v[1] / s;
    p_out[2] = p_v[2] / s;
}

inline float vec3_dot(const float* p_v1, const float* p_v2) {
    return p_v1[0] * p_v2[0] + p_v1[1] * p_v2[1] + p_v1[2] * p_v2[2];
}

inline float vec3_len_sq(const float* p_v) {
    return vec3_dot(p_v, p_v);
}

inline float vec3_len(const float* p_v) {
    return sqrtf(vec3_len_sq(p_v));
}

inline void vec3_norm(const float* p_v, float* p_out) {
    const float len = vec3_len(p_v);
    vec3_div_s(p_v, len, p_out);
}

inline void vec3_cross(const float* p_v1, const float* p_v2, float* p_out) {
    const float result[3] = {
        p_v1[1] * p_v2[2] - p_v1[2] * p_v2[1],
        p_v1[2] * p_v2[0] - p_v1[0] * p_v2[2],
        p_v1[0] * p_v2[1] - p_v1[1] * p_v2[0]
    };
    vec3_copy(result, p_out);
}

inline float vec3_dist_sq(const float* p_v1, const float* p_v2) {
    float tmp[3];
    vec3_sub(p_v2, p_v1, tmp);
    return vec3_len_sq(tmp);
}

inline float vec3_dist(const float* p_v1, const float* p_v2) {
    float tmp[3];
    vec3_sub(p_v2, p_v1, tmp);
    return vec3_len(tmp);
}

inline int vec3_cmp(const float* p_v1, const float* p_v2) {
    return FLT_CMP(p_v1[0], p_v2[0])
        && FLT_CMP(p_v1[1], p_v2[1])
        && FLT_CMP(p_v1[2], p_v2[2]);
}

inline int vec3_is_zero(const float* p_v) {
    return FLT_CMP(p_v[0], 0)
        && FLT_CMP(p_v[1], 0)
        && FLT_CMP(p_v[2], 0);
};

inline void vec3_min(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] < p_v2[0] ? p_v1[0] : p_v2[0];
    p_out[1] = p_v1[1] < p_v2[1] ? p_v1[1] : p_v2[1];
    p_out[2] = p_v1[2] < p_v2[2] ? p_v1[2] : p_v2[2];
}

inline void vec3_max(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] > p_v2[0] ? p_v1[0] : p_v2[0];
    p_out[1] = p_v1[1] > p_v2[1] ? p_v1[1] : p_v2[1];
    p_out[2] = p_v1[2] > p_v2[2] ? p_v1[2] : p_v2[2];
}

inline float vec3_minor(const float* p_v) {
    return
        p_v[0] < p_v[1] && p_v[0] < p_v[2] ? p_v[0]
        : p_v[1] < p_v[2] ? p_v[1]
        : p_v[2];
}

inline float vec3_major(const float* p_v) {
    return
        p_v[0] > p_v[1] && p_v[0] > p_v[2] ? p_v[0]
        : p_v[1] > p_v[2] ? p_v[1]
        : p_v[2];
}

inline float vec3_sum(const float* p_v) {
    return p_v[0] + p_v[1] + p_v[2];
}

inline void vec3_inv(const float* p_v, float* p_out) {
    p_out[0] = -p_v[0];
    p_out[1] = -p_v[1];
    p_out[2] = -p_v[2];
}

#endif
