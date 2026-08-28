#ifndef CX_PLATFORM_WINDOW_H
#define CX_PLATFORM_WINDOW_H

#include <stdint.h>

#include "cx_macro.h"
#include "cx_platform_input_state.h"
#include "cx_result.h"

#define CX_LOG_CAT_PLATFORM_WINDOW "platform:window"

struct cx_platform_window;

struct cx_platform_window {
	struct cx_platform_input_state input_state_;
	int b_was_focus_changed_;
	int b_was_resized_;
	CX_OPAQUE_INTERNALS(50);
};

cx_result cx_platform_window_create(
	uint32_t width, uint32_t height, const char* s_title, struct cx_platform_window* p_out);
void      cx_platform_window_destroy(struct cx_platform_window* p_window);
int       cx_platform_window_is_open(const struct cx_platform_window* p_window);
int       cx_platform_window_is_focused(const struct cx_platform_window* p_window);
void      cx_platform_window_size(
    const struct cx_platform_window* p_window, unsigned int* p_out_x, unsigned int* p_out_y);
void      cx_platform_window_process_events(struct cx_platform_window* p_window);
int       cx_platform_window_was_focus_changed(const struct cx_platform_window* p_window);
int       cx_platform_window_was_resized(const struct cx_platform_window* p_window);
const struct cx_platform_input_state* 
          cx_platform_window_input_state(const struct cx_platform_window* p_window);
void      cx_platform_window_normalize_client_coords(
	const struct cx_platform_window* p_window, int client_x, int client_y, float* p_x, float* p_y);
void      cx_platform_window_client_coords_to_ndc(
	const struct cx_platform_window* p_window, int client_x, int client_y, float* p_out_x, float* p_out_y);
void      cx_platform_window_client_to_world_ray(
	const struct cx_platform_window* p_window,
	const float* p_camera_matrix,
	int client_x, int client_y,
	float* p_out_ray);

#endif
