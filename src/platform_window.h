#ifndef PLATFORM_WINDOW_H
#define PLATFORM_WINDOW_H

#include <stdint.h>

#include "cx_error.h"
#include "cx_macro.h"
#include "keys.h"
#include "mouse_buttons.h"

#define CX_LOG_CAT_PLATFORM_WINDOW "platform:window"

struct platform_window {
	void(*f_callback_on_created_)(struct platform_window*, void*);
	void* p_callback_on_created_user_ptr_;
	void(*f_callback_on_close_)(struct platform_window*, void*);
	void* p_callback_on_close_user_ptr_;
	void(*f_callback_on_focus_change_)(struct platform_window*, void*, int);
	void* p_callback_on_focus_change_user_ptr_;
	void(*f_callback_on_resize_)(struct platform_window*, void*, uint32_t, uint32_t);
	void* p_callback_on_resize_user_ptr_;
	void(*f_callback_on_key_)(struct platform_window*, void*, enum key, int, unsigned int);
	void* p_callback_on_key_user_ptr_;
	void(*f_callback_on_mouse_button_)(struct platform_window*, void*, enum mouse_button, int, unsigned int);
	void* p_callback_on_mouse_button_user_ptr_;
	void(*f_callback_on_mouse_move_)(struct platform_window*, void*, int, int, unsigned int);
	void* p_callback_on_mouse_move_user_ptr_;
	void(*f_callback_on_scroll_)(struct platform_window*, void*, int, unsigned int);
	void* p_callback_on_scroll_user_ptr_;
	void(*f_callback_on_char_)(struct platform_window*, void*, unsigned int);
	void* p_callback_on_char_user_ptr_;
	int          mouse_pos_[2];
	int          mouse_pos_old_[2];
	unsigned int mods_;
	CX_OPAQUE_INTERNALS(50);
};

enum cx_error platform_window_create(
	uint32_t width, uint32_t height,
	const char* s_title,
	void(*f_callback_on_created)(struct platform_window*, void*),
	void* p_callback_on_created_user_ptr,
	struct platform_window* p_out_window);

void platform_window_destroy(struct platform_window* p_window);

void platform_window_poll_events(struct platform_window* p_window);

int  platform_window_is_open(const struct platform_window* p_window);

void platform_window_size(const struct platform_window* p_window, unsigned int* p_width, unsigned int* p_height);

void platform_window_get_mouse_client_coords(const struct platform_window* p_window, int* p_client_x, int* p_client_y);

void platform_window_get_mouse_client_coord_scaled(
	const struct platform_window* p_window, unsigned int width, unsigned int height, int* p_out_x, int* p_out_y);

void platform_window_normalize_client_coords(
	const struct platform_window* p_window,
	int client_x, int client_y,
	float* p_x, float* p_y);

void platform_window_client_coords_to_ndc(
	const struct platform_window* p_window,
	int client_x, int client_y,
	float* p_out_x, float* p_out_y);

void platform_window_client_to_world_ray(
	const struct platform_window* p_window,
	const float* p_camera_matrix,
	int client_x, int client_y,
	float* p_out_ray);

void platform_window_set_on_close_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*),
	void* p_user_ptr);

void platform_window_set_on_focus_change_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, int),
	void* p_user_ptr);

void platform_window_set_on_resize_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, uint32_t, uint32_t),
	void* p_user_ptr);

void platform_window_set_on_key_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, enum key, int, unsigned int),
	void* p_user_ptr);

void platform_window_set_on_mouse_button_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, enum mouse_button, int, unsigned int),
	void* p_user_ptr);

void platform_window_set_on_mouse_move_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, int, int, unsigned int),
	void* p_user_ptr);

void platform_window_set_on_scroll_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, int, unsigned int),
	void* p_user_ptr);

void platform_window_set_on_char_callback(
	struct platform_window* p_window,
	void(*f_callback)(struct platform_window*, void*, unsigned int),
	void* p_user_ptr);

#endif
