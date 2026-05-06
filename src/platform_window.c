#include "matrix.h"
#include "platform_window.h"
#include "vector.h"

void platform_window_get_mouse_client_coords(const struct platform_window* p_window, int* p_x, int* p_y) {
    *p_x = p_window->mouse_pos_[0];
    *p_y = p_window->mouse_pos_[1];
}

void platform_window_normalize_client_coords(
	const struct platform_window* p_window,
	int client_x, int client_y,
	float* p_x, float* p_y) {
    
	unsigned int width, height;
    platform_window_size(p_window, &width, &height);
    *p_x = (float)client_x / width;
    *p_y = (float)client_y / height;
}

void platform_window_client_coords_to_ndc(
	const struct platform_window* p_window,
	int client_x, int client_y,
	float* p_out_x, float* p_out_y) {
    
	platform_window_normalize_client_coords(p_window, client_x, client_y, p_out_x, p_out_y);
    *p_out_x =  (*p_out_x - 0.5) * 2;
    *p_out_y = -(*p_out_y - 0.5) * 2;
}

void platform_window_client_to_world_ray(
	const struct platform_window* p_window,
	const float* p_camera_matrix,
	int client_x, int client_y,
	float* p_out_ray) {
    
	float inv[16];
    matrix_inverse(4, p_camera_matrix, inv);
    
    float ndc[2];
    platform_window_client_coords_to_ndc(p_window, client_x, client_y, &ndc[0], &ndc[1]);

    float n[4] = { ndc[0], ndc[1], -1, 1 };
    matrix_multiply_vec4(inv, n, n);
    vec_div_s(4, n, n[3], n);
    
    float f[4] = { ndc[0], ndc[1], 1, 1 };
    matrix_multiply_vec4(inv, f, f);
    vec_div_s(4, f, f[3], f);

    vec3_sub(f, n, p_out_ray);
    vec3_norm(p_out_ray, p_out_ray);
}

void platform_window_set_on_close_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*),
	void* p_user_ptr) {
    
	p_window->f_callback_on_close_ = f_callback;
    p_window->p_callback_on_close_user_ptr_ = p_user_ptr;
}

void platform_window_set_on_focus_change_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, int),
	void* p_user_ptr) {
    
	p_window->f_callback_on_focus_change_ = f_callback;
    p_window->p_callback_on_focus_change_user_ptr_ = p_user_ptr;
}

void platform_window_set_on_resize_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, int, int),
	void* p_user_ptr) {
    
	p_window->f_callback_on_resize_ = f_callback;
    p_window->p_callback_on_resize_user_ptr_ = p_user_ptr;
}

void platform_window_set_on_key_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, enum key, int, unsigned int),
	void* p_user_ptr) {
    
	p_window->f_callback_on_key_ = f_callback;
    p_window->p_callback_on_key_user_ptr_ = p_user_ptr;
}

void platform_window_set_on_mouse_button_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, enum mouse_button, int, unsigned int),
	void* p_user_ptr) {
    
	p_window->f_callback_on_mouse_button_ = f_callback;
    p_window->p_callback_on_mouse_button_user_ptr_ = p_user_ptr;
}

void platform_window_set_on_mouse_move_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, int, int, unsigned int),
	void* p_user_ptr) {
    
	p_window->f_callback_on_mouse_move_ = f_callback;
    p_window->p_callback_on_mouse_move_user_ptr_ = p_user_ptr;
}

void platform_window_set_on_scroll_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, int, unsigned int),
	void* p_user_ptr) {
    
	p_window->f_callback_on_scroll_ = f_callback;
    p_window->p_callback_on_scroll_user_ptr_ = p_user_ptr;
}

void platform_window_set_on_char_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, unsigned int),
	void* p_user_ptr) {

    p_window->f_callback_on_char_ = f_callback;
    p_window->p_callback_on_char_user_ptr_ = p_user_ptr;
}

#ifdef PLATFORM_WIN32
#include "platform_window.win32.c"
#else
#include "platform_window.nix_x11.c"
#endif
