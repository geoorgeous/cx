#ifndef PLATFORM_WINDOW_H
#define PLATFORM_WINDOW_H

#include <stdint.h>

#include "cx_macro.h"
#include "cx_platform_input_state.h"
#include "cx_result.h"

#define CX_LOG_CAT_PLATFORM_WINDOW "platform:window"

struct platform_window;

struct platform_window {
	struct cx_platform_input_state input_state_;
	CX_OPAQUE_INTERNALS(50);
};

cx_result platform_window_create(
	uint32_t width, uint32_t height, const char* s_title, struct platform_window* p_out_window);

void platform_window_destroy(struct platform_window* p_window);

void platform_window_process_events(struct platform_window* p_window);

int platform_window_is_open(const struct platform_window* p_window);

int platform_window_is_focused(const struct platform_window* p_window);

void platform_window_size(const struct platform_window* p_window, unsigned int* p_out_x, unsigned int* p_out_y);

int platform_window_was_focus_changed(const struct platform_window* p_window);

int platform_window_was_resized(const struct platform_window* p_window);

const struct cx_platform_input_state* platform_window_input_state(const struct platform_window* p_window);

void platform_window_normalize_client_coords(
	const struct platform_window* p_window, int client_x, int client_y, float* p_x, float* p_y);

void platform_window_client_coords_to_ndc(
	const struct platform_window* p_window, int client_x, int client_y, float* p_out_x, float* p_out_y);

void platform_window_client_to_world_ray(
	const struct platform_window* p_window,
	const float* p_camera_matrix,
	int client_x, int client_y,
	float* p_out_ray);

#endif
