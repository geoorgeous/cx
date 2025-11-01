#include <math.h>
#include <string.h>

#include "vector.h"

void vec_set(size_t n, const float* p_v, float* p_result) {
    memcpy(p_result, p_v, sizeof(*p_v) * n);
}

void vec_set_s(size_t n, float s, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = s;
    }
}

void vec_clr(size_t n, float* p_result) {
    memset(p_result, 0, sizeof(*p_result) * n);
}

void vec_add(size_t n, const float* p_v1, const float* p_v2, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v1[i] + p_v2[i];
    }
}

void vec_add_s(size_t n, const float* p_v, float s, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v[i] + s;
    }
}

void vec_sub(size_t n, const float* p_v1, const float* p_v2, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v1[i] - p_v2[i];
    }
}

void vec_sub_s(size_t n, const float* p_v, float s, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v[i] - s;
    }
}

void vec_mul(size_t n, const float* p_v1, const float* p_v2, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v1[i] * p_v2[i];
    }
}

void vec_mul_s(size_t n, const float* p_v, float s, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v[i] * s;
    }
}

void vec_div(size_t n, const float* p_v1, const float* p_v2, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v1[i] / p_v2[i];
    }
}

void vec_div_s(size_t n, const float* p_v, float s, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v[i] / s;
    }
}

float vec_dot(size_t n, const float* p_v1, const float* p_v2) {
    float result = 0;
    for (size_t i = 0; i < n; ++i)
        result += p_v1[i] * p_v2[i];
    return result;
}

float vec_len(size_t n, const float* p_v) {
    return sqrtf(vec_len_sq(n, p_v));
}

float vec_len_sq(size_t n, const float* p_v) {
    return vec_dot(n, p_v, p_v);
}

void vec_norm(size_t n, const float* p_v, float* p_result) {
    vec_div_s(n, p_v, vec_len(n, p_v), p_result);
}

int vec_cmp(size_t n, const float* p_v1, const float* p_v2) {
    for (size_t i = 0; i < n; ++i) {
        if (!FLT_CMP(p_v1[i], p_v2[i])) {
            return 0;
        }
    }
    return 1;
}

int vec_is_zero(size_t n, const float* p_v) {
    for (size_t i = 0; i < n; ++i) {
        if (!FLT_CMP(p_v[i], 0)) {
            return 0;
        }
    }
    return 1;
}

void vec_min(size_t n, const float* p_v1, const float* p_v2, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v1[i] < p_v2[i] ? p_v1[i] : p_v2[i];
    }
}

void vec_max(size_t n, const float* p_v1, const float* p_v2, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = p_v1[i] > p_v2[i] ? p_v1[i] : p_v2[i];
    }
}

float vec_major(size_t n, const float* p_v) {
    float m = p_v[0];
    for (size_t i = 1; i < n; ++i) {
        if (p_v[i] > m) {
            m = p_v[i];
        }
    }
    return m;
}

float vec_minor(size_t n, const float* p_v) {
    float m = p_v[0];
    for (size_t i = 1; i < n; ++i) {
        if (p_v[i] < m) {
            m = p_v[i];
        }
    }
    return m;
}

float vec_sum(size_t n, const float* p_v) {
    float s = 0;
    for (size_t i = 1; i < n; ++i) {
        s += p_v[i];
    }
    return s;
}

void vec_inv(size_t n, const float* p_v, float* p_result) {
    for (size_t i = 0; i < n; ++i) {
        p_result[i] = -p_v[i];
    }
}

extern inline void vec3_set(const float* p_v, float* p_result);
extern inline void vec3_set_s(float s, float* p_result);
extern inline void vec3_set_ijk(float i, float j, float k, float* p_result);
extern inline void vec3_clr(float* p_result);
extern inline void vec3_add(const float* p_v1, const float* p_v2, float* p_result);
extern inline void vec3_add_s(const float* p_v, float s, float* p_result);
extern inline void vec3_sub(const float* p_v1, const float* p_v2, float* p_result);
extern inline void vec3_sub_s(const float* p_v, float s, float* p_result);
extern inline void vec3_mul(const float* p_v1, const float* p_v2, float* p_result);
extern inline void vec3_mul_s(const float* p_v, float s, float* p_result);
extern inline void vec3_div(const float* p_v1, const float* p_v2, float* p_result);
extern inline void vec3_div_s(const float* p_v, float s, float* p_result);
extern inline float vec3_dot(const float* p_v1, const float* p_v2);
extern inline float vec3_len_sq(const float* p_v);
extern inline float vec3_len(const float* p_v);
extern inline void vec3_norm(const float* p_v, float* p_result);
extern inline void vec3_cross(const float* p_v1, const float* p_v2, float* p_result);
extern inline float vec3_dist_sq(const float* p_v1, const float* p_v2);
extern inline float vec3_dist(const float* p_v1, const float* p_v2);
extern inline int vec3_cmp(const float* p_v1, const float* p_v2);
extern inline int vec3_is_zero(const float* p_v);
extern inline void vec3_min(const float* p_v1, const float* p_v2, float* p_result);
extern inline void vec3_max(const float* p_v1, const float* p_v2, float* p_result);
extern inline float vec3_minor(const float* p_v);
extern inline float vec3_major(const float* p_v);
extern inline float vec3_sum(const float* p_v);
extern inline void vec3_inv(const float* p_v, float* p_result);