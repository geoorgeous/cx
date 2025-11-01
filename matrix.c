#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "math_utils.h"
#include "matrix.h"
#include "vector.h"

// Note: matrices are expected in column-major order

#define MAT_ELEM(MAT, N, COL, ROW) (MAT[((N)*(COL))+(ROW)])
#define MAT4_ELEM(MAT, COL, ROW) MAT_ELEM(MAT, 4, COL, ROW)

void matrix_copy(const float* p_m, float* p_result) {
    memcpy(p_result, p_m, sizeof(*p_m) * 16);
}

void matrix_make_identity(float* p_result) {
    memset(p_result, 0.f, sizeof(*p_result) * 16);
    p_result[0] = p_result[5] = p_result[10] = p_result[15] = 1.f;
}

void matrix_make_translation(float t_x, float t_y, float t_z, float* p_result) {
    memset(p_result, 0.f, sizeof(*p_result) * 16);
    p_result[12] = t_x;
    p_result[13] = t_y;
    p_result[14] = t_z;
    p_result[ 0] = p_result[5] = p_result[10] = p_result[15] = 1;
}

void matrix_make_scale(float s_x, float s_y, float s_z, float* p_result) {
    memset(p_result, 0.f, sizeof(*p_result) * 16);
    p_result[ 0] = s_x;
    p_result[ 5] = s_y;
    p_result[10] = s_z;
    p_result[15] = 1.f;
}

void matrix_make_uniform_scale(float s, float* p_result) {
    matrix_make_scale(s, s, s, p_result);
}

void matrix_make_rotation_x(float r, float* p_result) {
    const float cos_r = cosf(r);
    const float sin_r = sinf(r);
    memset(p_result, 0.f, sizeof(*p_result) * 16);
    p_result[ 5] =  cos_r;
    p_result[ 6] =  sin_r;
    p_result[ 9] = -sin_r;
    p_result[10] =  cos_r;
    p_result[ 0] = p_result[15] = 1.f;
}

void matrix_make_rotation_y(float r, float* p_result) {
    const float cos_r = cosf(r);
    const float sin_r = sinf(r);
    memset(p_result, 0.f, sizeof(*p_result) * 16);
    p_result[ 0] =  cos_r;
    p_result[ 2] = -sin_r;
    p_result[ 8] =  sin_r;
    p_result[10] =  cos_r;
    p_result[ 5] = p_result[15] = 1.f;
}

void matrix_make_rotation_z(float r, float* p_result) {
    const float cos_r = cosf(r);
    const float sin_r = sinf(r);
    memset(p_result, 0.f, sizeof(*p_result) * 16);
    p_result[ 0] =  cos_r;
    p_result[ 1] =  sin_r;
    p_result[ 4] = -sin_r;
    p_result[ 5] =  cos_r;
    p_result[10] = p_result[15] = 1.f;
}

void matrix_make_rotation_from_quaternion(const float* p_quaternion, float* p_result) {
    const float ij = p_quaternion[0] * p_quaternion[1];
    const float ik = p_quaternion[0] * p_quaternion[2];
    const float iw = p_quaternion[0] * p_quaternion[3];
    const float jk = p_quaternion[1] * p_quaternion[2];
    const float jw = p_quaternion[1] * p_quaternion[3];
    const float kw = p_quaternion[2] * p_quaternion[3];
    const float ww = p_quaternion[3] * p_quaternion[3];

    p_result[ 0] = 2 * (ww + p_quaternion[0]*p_quaternion[0]) - 1;
    p_result[ 4] = 2 * (ij - kw);
    p_result[ 8] = 2 * (ik + jw);

    p_result[ 1] = 2 * (ij + kw);
    p_result[ 5] = 2 * (ww + p_quaternion[1]*p_quaternion[1]) - 1;
    p_result[ 9] = 2 * (jk - iw);

    p_result[ 2] = 2 * (ik - jw);
    p_result[ 6] = 2 * (jk + iw);
    p_result[10] = 2 * (ww + p_quaternion[2]*p_quaternion[2]) - 1;

    p_result[ 3] = p_result[7] = p_result[11] = p_result[12] = p_result[13] = p_result[14] = 0.f;
    p_result[15] = 1.f;
}

void matrix_make_orthographic_projection(float left, float right, float top, float bottom, float n, float f, float* p_result) {
    const float width = right - left;
	const float height = top - bottom;
	const float depth = f - n;
	
	p_result[ 0] = 2 / width;
	p_result[ 5] = 2 / height;
	p_result[10] = -2 / depth;
    
	p_result[12] = -(right + left) / width;
	p_result[13] = -(top + bottom) / height;
	p_result[14] = -(f + n) / depth;

	p_result[ 1] = p_result[ 2] = p_result[ 3] =
	p_result[ 4] = p_result[ 6] = p_result[ 7] =
    p_result[ 8] = p_result[ 9] = p_result[11] = 0;
	p_result[15] = 1;
}

void matrix_make_perspective_projection(float vfov, float aspect, float n, float f, float* p_result) {
	const float tanhvfov = tanf(vfov * 0.5f);
	const float depth = f - n;
	
	p_result[ 0] = 1 / (aspect * tanhvfov);
	p_result[ 5] = 1 / tanhvfov;
	p_result[10] = -(f + n) / depth;
    
	p_result[14] = -(2 * f * n) / depth;
    
	p_result[ 1] = p_result[ 2] = p_result[ 3] =
	p_result[ 4] = p_result[ 6] = p_result[ 7] =
	p_result[ 8] = p_result[ 9] =
    p_result[12] = p_result[13] = p_result[15] = 0;
	p_result[11] = -1;
}

void matrix_make_lookat(const float* p_pos, const float* p_target, const float* p_up, float* p_result) {
    float f[3];
    vec3_sub(p_pos, p_target, f);
    vec3_norm(f, f);

    float r[3];
    vec3_cross(p_up, f, r);
    vec3_norm(r, r);

    float u[3];
    vec3_cross(f, r, u);
    vec3_norm(u, u);

    float t[3];
    t[0] = vec3_dot(p_pos, r);
    t[1] = vec3_dot(p_pos, p_up);
    t[2] = vec3_dot(p_pos, f);

    p_result[ 0] = r[0];
    p_result[ 4] = r[1];
    p_result[ 8] = r[2];
    
    p_result[ 1] = u[0];
    p_result[ 5] = u[1];
    p_result[ 9] = u[2];
    
    p_result[ 2] = f[0];
    p_result[ 6] = f[1];
    p_result[10] = f[2];
    
    p_result[12] = t[0];
    p_result[13] = t[1];
    p_result[14] = t[2];

    p_result[ 3] =
    p_result[ 7] =
    p_result[11] = 0;
    p_result[15] = 1;
}

void matrix_multiply(const float* p_m1, const float* p_m2, float* p_result) {
    float result[16] = {0};
    for (int n = 0; n < 4; ++n) {
        for (int m = 0; m < 4; ++m) {
            for (int i = 0; i < 4; ++i) {
                MAT4_ELEM(result, n, m) += MAT4_ELEM(p_m1, i, m) * MAT4_ELEM(p_m2, n, i);
            }
        }
    }
    memcpy(p_result, result, sizeof(*result) * 16);
}

void matrix_multiply_vec4(const float* p_m, const float* p_v, float* p_result) {
    float result[] = {
        MAT4_ELEM(p_m, 0, 0) * p_v[0] +
        MAT4_ELEM(p_m, 1, 0) * p_v[1] +
        MAT4_ELEM(p_m, 2, 0) * p_v[2] +
        MAT4_ELEM(p_m, 3, 0) * p_v[3],

        MAT4_ELEM(p_m, 0, 1) * p_v[0] +
        MAT4_ELEM(p_m, 1, 1) * p_v[1] +
        MAT4_ELEM(p_m, 2, 1) * p_v[2] +
        MAT4_ELEM(p_m, 3, 1) * p_v[3],

        MAT4_ELEM(p_m, 0, 2) * p_v[0] +
        MAT4_ELEM(p_m, 1, 2) * p_v[1] +
        MAT4_ELEM(p_m, 2, 2) * p_v[2] +
        MAT4_ELEM(p_m, 3, 2) * p_v[3],

        MAT4_ELEM(p_m, 0, 3) * p_v[0] +
        MAT4_ELEM(p_m, 1, 3) * p_v[1] +
        MAT4_ELEM(p_m, 2, 3) * p_v[2] +
        MAT4_ELEM(p_m, 3, 3) * p_v[3],
    };
    p_result[0] = result[0];
    p_result[1] = result[1];
    p_result[2] = result[2];
    p_result[3] = result[3];
}

void matrix_multiply_vec3(const float* p_m, const float* p_v, float* p_result) {
    float result[] = {
        MAT4_ELEM(p_m, 0, 0) * p_v[0] +
        MAT4_ELEM(p_m, 1, 0) * p_v[1] +
        MAT4_ELEM(p_m, 2, 0) * p_v[2] +
        MAT4_ELEM(p_m, 3, 0),

        MAT4_ELEM(p_m, 0, 1) * p_v[0] +
        MAT4_ELEM(p_m, 1, 1) * p_v[1] +
        MAT4_ELEM(p_m, 2, 1) * p_v[2] +
        MAT4_ELEM(p_m, 3, 1),

        MAT4_ELEM(p_m, 0, 2) * p_v[0] +
        MAT4_ELEM(p_m, 1, 2) * p_v[1] +
        MAT4_ELEM(p_m, 2, 2) * p_v[2] +
        MAT4_ELEM(p_m, 3, 2)
    };
    p_result[0] = result[0];
    p_result[1] = result[1];
    p_result[2] = result[2];
}

void matrix_decompose_trs(const float* p_m, float* p_t, float* p_r, float* p_s) {
    vec3_set(&p_m[12], p_t);

    // todo: proper decomposition including negative scale, maybe see gltf 2.0 spec

    p_s[0] = p_m[ 0];
    p_s[1] = p_m[ 5];
    p_s[2] = p_m[10];

    quaternion_identity(p_r);
}

float matrix_determinant(size_t n, const float* p_m) {
    if (n == 0) {
        return 1;
    }
    
    if (n == 1) {
        return p_m[0];
    }

    if (n == 2) {
        return p_m[0] * p_m[3] - p_m[2] * p_m[1];
    }

    float subm[16];

    float result = 0;
    int sign = 1;
    for (size_t i = 0; i < n; ++i) {
        for (size_t mi = 1; mi < n; ++mi) {
            size_t z = 0;
            for (size_t ni = 0; ni < n; ++ni) {
                if (ni == i) {
                    continue;
                }
                MAT_ELEM(subm, n - 1, z, mi - 1) = MAT_ELEM(p_m, n, ni, mi);
                ++z;
            }
        }

        result += sign * p_m[n * i] * matrix_determinant(n - 1, subm);
        sign = -sign;
    }

    return result;
}

void matrix_transpose(size_t n, const float* p_m, float* p_result) {
    float result[16];
    for (size_t ni = 0; ni < n; ++ni) {
        for (size_t mi = 0; mi < n; ++mi) {
            MAT_ELEM(result, n, ni, mi) = MAT_ELEM(p_m, n, mi, ni);
        }
    }
    memcpy(p_result, result, sizeof(*result) * n * n);
}

void matrix_cofactor(size_t n, const float* p_m, float* p_result) {
    float result[16];
    float subm[16];

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            size_t p = 0;
            for (size_t x = 0; x < n; ++x) {
                if (x == i) {
                    continue;
                }

                size_t q = 0;
                for (size_t y = 0; y < n; ++y) {
                    if (y == j) {
                        continue;
                    }

                    MAT_ELEM(subm, n - 1, p, q) = MAT_ELEM(p_m, n, x, y);
                    ++q;
                }
                ++p;
            }
            MAT_ELEM(result, n, i, j) = powf(-1, i + j) * matrix_determinant(n - 1, subm);
        }
    }
    memcpy(p_result, result, sizeof(*result) * n * n);
}

void matrix_inverse(size_t n, const float* p_m, float* p_result) {
    const float d = matrix_determinant(n, p_m);

    if (FLT_CMP(d, 0)) {
        return;
    }

    matrix_cofactor(n, p_m, p_result);
    matrix_transpose(n, p_result, p_result);
    
    const float dinv = 1.0f / d;
    matrix_multiply_s(n, n, p_result, dinv, p_result);
}

void matrix_add(size_t n, size_t m, const float* p_m1, const float* p_m2, float* p_result) {
    for (size_t ni = 0; ni < n; ++ni) {
        for (size_t mi = 0; mi < m; ++mi) {
            MAT_ELEM(p_result, n, ni, mi) = MAT_ELEM(p_m1, n, ni, mi) + MAT_ELEM(p_m2, n, ni, mi);
        }
    }
}

void matrix_multiply_s(size_t n, size_t m, const float* p_m1, float s, float* p_result) {
    for (size_t ni = 0; ni < n; ++ni) {
        for (size_t mi = 0; mi < m; ++mi) {
            MAT_ELEM(p_result, n, ni, mi) = MAT_ELEM(p_m1, n, ni, mi) * s;
        }
    }
}

void quaternion_identity(float* p_quaternion) {
    p_quaternion[0] = 
    p_quaternion[1] = 
    p_quaternion[2] = 0;
    p_quaternion[3] = 1;
}

void quaternion_conjugate(const float* p_quaternion, float* p_result) {
    p_result[0] = -p_quaternion[0];
    p_result[1] = -p_quaternion[1];
    p_result[2] = -p_quaternion[2];
    p_result[3] =  p_quaternion[3];
}

void quaternion_from_axis_angle(const float* p_axis, float angle, float* p_result) {
    const float half_angle = angle * 0.5f;
    const float s = sinf(half_angle);
    vec3_mul_s(p_axis, s, p_result);
    p_result[3] = cosf(half_angle);
}

void quaternion_multiply(const float* p_q1, const float* p_q2, float* p_result) {
    float result[4];
    result[0] = p_q1[3] * p_q2[0] + p_q1[0] * p_q2[3] + p_q1[1] * p_q2[2] - p_q1[2] * p_q2[1];
    result[1] = p_q1[3] * p_q2[1] - p_q1[0] * p_q2[2] + p_q1[1] * p_q2[3] + p_q1[2] * p_q2[0];
    result[2] = p_q1[3] * p_q2[2] + p_q1[0] * p_q2[1] - p_q1[1] * p_q2[0] + p_q1[2] * p_q2[3];
    result[3] = p_q1[3] * p_q2[3] - p_q1[0] * p_q2[0] - p_q1[1] * p_q2[1] - p_q1[2] * p_q2[2];
    p_result[0] = result[0];
    p_result[1] = result[1];
    p_result[2] = result[2];
    p_result[3] = result[3];
}

void quaternion_rotate_vec3(const float* p_q, const float* p_v, float* p_result) {
    float temp[3];
    
    vec3_mul_s(p_v, p_q[3] * p_q[3] - vec3_dot(p_q, p_q), temp);

    vec3_cross(p_q, p_v, p_result);
    vec3_mul_s(p_result, 2 * p_q[3], p_result);

    vec3_add(temp, p_result, p_result);
    
    vec3_mul_s(p_q, 2 * vec3_dot(p_q, p_v), temp);
    
    vec3_add(temp, p_result, p_result);
}