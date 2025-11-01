#ifndef _H__MATRIX
#define _H__MATRIX

#include <stdint.h>

void matrix_copy(const float* p_m, float* p_result);
void matrix_make_identity(float* p_result);
void matrix_make_translation(float t_x, float t_y, float t_z, float* p_result);
void matrix_make_scale(float s_x, float s_y, float s_z, float* p_result);
void matrix_make_uniform_scale(float s, float* p_result);
void matrix_make_rotation_x(float r, float* p_result);
void matrix_make_rotation_y(float r, float* p_result);
void matrix_make_rotation_z(float r, float* p_result);
void matrix_make_rotation_from_quaternion(const float* p_quaternion, float* p_result);
void matrix_make_orthographic_projection(float left, float right, float top, float bottom, float n, float f, float* p_result);
void matrix_make_perspective_projection(float vfov, float aspect, float n, float f, float* p_result);
void matrix_make_lookat(const float* p_pos, const float* p_target, const float* p_up, float* p_result);
void matrix_multiply(const float* p_m1, const float* p_m2, float* p_result);
void matrix_multiply_vec4(const float* p_m, const float* p_v, float* p_result);
void matrix_multiply_vec3(const float* p_m, const float* p_v, float* p_result);
void matrix_decompose_trs(const float* p_m, float* p_t, float* p_r, float* p_s);

float matrix_determinant(size_t n, const float* p_m);
void  matrix_transpose(size_t n, const float* p_m, float* p_result);
void  matrix_cofactor(size_t n, const float* p_m, float* p_result);
void  matrix_inverse(size_t n, const float* p_m, float* p_result);

void matrix_add(size_t n, size_t m, const float* p_m1, const float* p_m2, float* p_result);
void matrix_multiply_s(size_t n, size_t m, const float* p_m1, float s, float* p_result);

void quaternion_identity(float* p_quaternion);
void quaternion_conjugate(const float* p_quaternion, float* p_result);
void quaternion_from_axis_angle(const float* p_axis, float angle, float* p_result);
void quaternion_multiply(const float* p_q1, const float* p_q2, float* p_result);
void quaternion_rotate_vec3(const float* p_q, const float* p_v, float* p_result);

#endif