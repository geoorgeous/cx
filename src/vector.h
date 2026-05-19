#ifndef VECTOR_H
#define VECTOR_H

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "math_utils.h"

static inline void vec_copy(size_t n, const float* p_v, float* p_out) {
    memcpy(p_out, p_v, sizeof(*p_v) * n);
}

static inline void vec_set(size_t n, float s, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = s;
    }
}

static inline void vec_clr(size_t n, float* p_out) {
    memset(p_out, 0, sizeof(*p_out) * n);
}

static inline void vec_add(size_t n, const float* p_v1, const float* p_v2, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v1[i] + p_v2[i];
    }
}

static inline void vec_add_s(size_t n, const float* p_v, float s, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v[i] + s;
    }
}

static inline void vec_sub(size_t n, const float* p_v1, const float* p_v2, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v1[i] - p_v2[i];
    }
}

static inline void vec_sub_s(size_t n, const float* p_v, float s, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v[i] - s;
    }
}

static inline void vec_mul(size_t n, const float* p_v1, const float* p_v2, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v1[i] * p_v2[i];
    }
}

static inline void vec_mul_s(size_t n, const float* p_v, float s, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v[i] * s;
    }
}

static inline void vec_div(size_t n, const float* p_v1, const float* p_v2, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v1[i] / p_v2[i];
    }
}

static inline void vec_div_s(size_t n, const float* p_v, float s, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v[i] / s;
    }
}

static inline float vec_dot(size_t n, const float* p_v1, const float* p_v2) {
    float result = 0;
    for (size_t i = 0; i < n; ++i)
        result += p_v1[i] * p_v2[i];
    return result;
}

static inline float vec_len_sq(size_t n, const float* p_v) {
    return vec_dot(n, p_v, p_v);
}

static inline float vec_len(size_t n, const float* p_v) {
    return sqrtf(vec_len_sq(n, p_v));
}

static inline void vec_norm(size_t n, const float* p_v, float* p_out) {
    vec_div_s(n, p_v, vec_len(n, p_v), p_out);
}

static inline int vec_cmp(size_t n, const float* p_v1, const float* p_v2) {
    for (size_t i = 0; i < n; ++i) {
        if (!FLT_CMP(p_v1[i], p_v2[i])) {
            return 0;
        }
    }
    return 1;
}

static inline int vec_is_zero(size_t n, const float* p_v) {
    for (size_t i = 0; i < n; ++i) {
        if (!FLT_CMP(p_v[i], 0)) {
            return 0;
        }
    }
    return 1;
}

static inline void vec_min(size_t n, const float* p_v1, const float* p_v2, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v1[i] < p_v2[i] ? p_v1[i] : p_v2[i];
    }
}

static inline void vec_max(size_t n, const float* p_v1, const float* p_v2, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = p_v1[i] > p_v2[i] ? p_v1[i] : p_v2[i];
    }
}

static inline float vec_major(size_t n, const float* p_v) {
    float m = p_v[0];
    for (size_t i = 1; i < n; ++i) {
        if (p_v[i] > m) {
            m = p_v[i];
        }
    }
    return m;
}

static inline float vec_minor(size_t n, const float* p_v) {
    float m = p_v[0];
    for (size_t i = 1; i < n; ++i) {
        if (p_v[i] < m) {
            m = p_v[i];
        }
    }
    return m;
}

static inline float vec_sum(size_t n, const float* p_v) {
    float s = 0;
    for (size_t i = 1; i < n; ++i) {
        s += p_v[i];
    }
    return s;
}

static inline void vec_inv(size_t n, const float* p_v, float* p_out) {
    for (size_t i = 0; i < n; ++i) {
        p_out[i] = -p_v[i];
    }
}

static inline void vec3_copy(const float* p_v, float* p_out) {
    p_out[0] = p_v[0];
    p_out[1] = p_v[1];
    p_out[2] = p_v[2];
}

static inline void vec3_set_s(float s, float* p_out) {
    p_out[0] =
    p_out[1] =
    p_out[2] = s;
}

static inline void vec3_set(float i, float j, float k, float* p_out) {
    p_out[0] = i;
    p_out[1] = j;
    p_out[2] = k;
}

static inline void vec3_clr(float* p_out) {
    vec3_set_s(0, p_out);
}

static inline void vec3_add(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] + p_v2[0];
    p_out[1] = p_v1[1] + p_v2[1];
    p_out[2] = p_v1[2] + p_v2[2];
}

static inline void vec3_add_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] + s;
    p_out[1] = p_v[1] + s;
    p_out[2] = p_v[2] + s;
}

static inline void vec3_sub(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] - p_v2[0];
    p_out[1] = p_v1[1] - p_v2[1];
    p_out[2] = p_v1[2] - p_v2[2];
}

static inline void vec3_sub_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] - s;
    p_out[1] = p_v[1] - s;
    p_out[2] = p_v[2] - s;
}

static inline void vec3_mul(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] * p_v2[0];
    p_out[1] = p_v1[1] * p_v2[1];
    p_out[2] = p_v1[2] * p_v2[2];
}

static inline void vec3_mul_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] * s;
    p_out[1] = p_v[1] * s;
    p_out[2] = p_v[2] * s;
}

static inline void vec3_div(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] / p_v2[0];
    p_out[1] = p_v1[1] / p_v2[1];
    p_out[2] = p_v1[2] / p_v2[2];
}

static inline void vec3_div_s(const float* p_v, float s, float* p_out) {
    p_out[0] = p_v[0] / s;
    p_out[1] = p_v[1] / s;
    p_out[2] = p_v[2] / s;
}

static inline float vec3_dot(const float* p_v1, const float* p_v2) {
    return p_v1[0] * p_v2[0] + p_v1[1] * p_v2[1] + p_v1[2] * p_v2[2];
}

static inline float vec3_len_sq(const float* p_v) {
    return vec3_dot(p_v, p_v);
}

static inline float vec3_len(const float* p_v) {
    return sqrtf(vec3_len_sq(p_v));
}

static inline void vec3_norm(const float* p_v, float* p_out) {
    const float len = vec3_len(p_v);
    vec3_div_s(p_v, len, p_out);
}

static inline void vec3_cross(const float* p_v1, const float* p_v2, float* p_out) {
    const float result[3] = {
        p_v1[1] * p_v2[2] - p_v1[2] * p_v2[1],
        p_v1[2] * p_v2[0] - p_v1[0] * p_v2[2],
        p_v1[0] * p_v2[1] - p_v1[1] * p_v2[0]
    };
    vec3_copy(result, p_out);
}

static inline float vec3_dist_sq(const float* p_v1, const float* p_v2) {
    float tmp[3];
    vec3_sub(p_v2, p_v1, tmp);
    return vec3_len_sq(tmp);
}

static inline float vec3_dist(const float* p_v1, const float* p_v2) {
    float tmp[3];
    vec3_sub(p_v2, p_v1, tmp);
    return vec3_len(tmp);
}

static inline int vec3_cmp(const float* p_v1, const float* p_v2) {
    return FLT_CMP(p_v1[0], p_v2[0])
        && FLT_CMP(p_v1[1], p_v2[1])
        && FLT_CMP(p_v1[2], p_v2[2]);
}

static inline int vec3_is_zero(const float* p_v) {
    return FLT_CMP(p_v[0], 0)
        && FLT_CMP(p_v[1], 0)
        && FLT_CMP(p_v[2], 0);
};

static inline void vec3_min(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] < p_v2[0] ? p_v1[0] : p_v2[0];
    p_out[1] = p_v1[1] < p_v2[1] ? p_v1[1] : p_v2[1];
    p_out[2] = p_v1[2] < p_v2[2] ? p_v1[2] : p_v2[2];
}

static inline void vec3_max(const float* p_v1, const float* p_v2, float* p_out) {
    p_out[0] = p_v1[0] > p_v2[0] ? p_v1[0] : p_v2[0];
    p_out[1] = p_v1[1] > p_v2[1] ? p_v1[1] : p_v2[1];
    p_out[2] = p_v1[2] > p_v2[2] ? p_v1[2] : p_v2[2];
}

static inline float vec3_minor(const float* p_v) {
    return
        p_v[0] < p_v[1] && p_v[0] < p_v[2] ? p_v[0]
        : p_v[1] < p_v[2] ? p_v[1]
        : p_v[2];
}

static inline float vec3_major(const float* p_v) {
    return
        p_v[0] > p_v[1] && p_v[0] > p_v[2] ? p_v[0]
        : p_v[1] > p_v[2] ? p_v[1]
        : p_v[2];
}

static inline float vec3_sum(const float* p_v) {
    return p_v[0] + p_v[1] + p_v[2];
}

static inline void vec3_inv(const float* p_v, float* p_out) {
    p_out[0] = -p_v[0];
    p_out[1] = -p_v[1];
    p_out[2] = -p_v[2];
}

/*
 * Orthonormal basis construction from:
 *
 * Jeppe Revall Frisvad,
 * "Building an Orthonormal Basis from a 3D Unit Vector
 * Without Normalization",
 * Journal of Graphics Tools, 2012.
 *
 * p_n must be normalized.
 */
static inline void vec3_orthonormal_basis(const float* p_n, float* p_out_t, float* p_out_b) {
	// Frisvad 2012
	const float sign = copysignf(1.f, p_n[2]);
	const float inv = -1.f / (sign + p_n[2]);
	const float invxy = p_n[0] * p_n[1] * inv;

	p_out_t[0] = 1.f + sign * p_n[0] * p_n[0] * inv;
	p_out_t[1] = sign * invxy;
	p_out_t[2] = -sign * p_n[0];

	p_out_b[0] = invxy;
	p_out_b[1] = sign + p_n[1] * p_n[1] * inv;
	p_out_b[2] = -p_n[1];
}

#endif
