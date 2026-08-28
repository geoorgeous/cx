#include "cx_platform_window.h"
#include "matrix.h"
#include "vector.h"

int cx_platform_window_was_focus_changed(const struct cx_platform_window* p_window) {
	return p_window->b_was_focus_changed_;
}

int cx_platform_window_was_resized(const struct cx_platform_window* p_window) {
	return p_window->b_was_resized_;
}

const struct cx_platform_input_state* cx_platform_window_input_state(const struct cx_platform_window* p_window) {
	return &p_window->input_state_;
}

void cx_platform_window_normalize_client_coords(
	const struct cx_platform_window* p_window, int client_x, int client_y, float* p_x, float* p_y) {
	
	unsigned int width, height;
	cx_platform_window_size(p_window, &width, &height);
	*p_x = (float)client_x / (float)width;
	*p_y = (float)client_y / (float)height;
}

void cx_platform_window_client_coords_to_ndc(
	const struct cx_platform_window* p_window, int client_x, int client_y, float* p_out_x, float* p_out_y) {
	
	cx_platform_window_normalize_client_coords(p_window, client_x, client_y, p_out_x, p_out_y);
	*p_out_x =  (*p_out_x - 0.5f) * 2;
	*p_out_y = -(*p_out_y - 0.5f) * 2;
}

void cx_platform_window_client_to_world_ray(
	const struct cx_platform_window* p_window,
	const float* p_camera_matrix,
	int client_x, int client_y,
	float* p_out_ray) {
	
	float inv[16];
	matrix_inverse(4, p_camera_matrix, inv);
	
	float ndc[2];
	cx_platform_window_client_coords_to_ndc(p_window, client_x, client_y, &ndc[0], &ndc[1]);

	float n[4] = { ndc[0], ndc[1], -1, 1 };
	matrix_multiply_vec4(inv, n, n);
	vec_div_s(4, n, n[3], n);
	
	float f[4] = { ndc[0], ndc[1], 1, 1 };
	matrix_multiply_vec4(inv, f, f);
	vec_div_s(4, f, f[3], f);

	vec3_sub(f, n, p_out_ray);
	vec3_norm(p_out_ray, p_out_ray);
}

#ifdef PLATFORM_WIN32
#include "platform_window.win32.c"
#else
#include "cx_platform_window.x11.c"
#endif
