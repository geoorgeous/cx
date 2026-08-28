#include "cx_dbg.h"
#include "cx_input.h"
#include "cx_platform_input_state.h"
#include "cx_platform_window.h"

static const struct cx_platform_input_state* g_p_input_state_present;
static struct cx_platform_input_state g_input_state_previous;
static int g_mouse_delta_x;
static int g_mouse_delta_y;

void cx_input_sample(const struct cx_platform_window* p_window) {
	g_p_input_state_present = cx_platform_window_input_state(p_window);

	g_mouse_delta_x = g_p_input_state_present->mouse_x - g_input_state_previous.mouse_x;
	g_mouse_delta_y = g_p_input_state_present->mouse_y - g_input_state_previous.mouse_y;

	g_input_state_previous = *g_p_input_state_present;
}

int cx_input_is_key_down(enum cx_key key) {
	CX_ASSERT(key >= 0 && key < CX_KEY_MAX_, INPUT);
	return g_p_input_state_present->keys[key].b_is_down;
}

int cx_input_is_button_down(enum cx_button button) {
	CX_ASSERT(button >= 0 && button < CX_BUTTON_MAX_, INPUT);
	return g_p_input_state_present->buttons[button].b_is_down;
}

void cx_input_mouse_position(int* p_out_x, int* p_out_y) {
	*p_out_x = g_p_input_state_present->mouse_x;
	*p_out_y = g_p_input_state_present->mouse_y;
}

void cx_input_mouse_delta(int* p_out_x, int* p_out_y) {
	*p_out_x = g_mouse_delta_x;
	*p_out_y = g_mouse_delta_y;
}

void cx_input_scroll_accum(int* p_out_x, int* p_out_y) {
	*p_out_x = g_p_input_state_present->scroll_accum_x;
	*p_out_y = g_p_input_state_present->scroll_accum_y;
}

int cx_input_was_key_pressed(enum cx_key key) {
	CX_ASSERT(key >= 0 && key < CX_KEY_MAX_, INPUT);
	return g_p_input_state_present->keys[key].b_is_down && !g_input_state_previous.keys[key].b_is_down;
}

int cx_input_was_key_released(enum cx_key key) {
	CX_ASSERT(key >= 0 && key < CX_KEY_MAX_, INPUT);
	return !g_p_input_state_present->keys[key].b_is_down && g_input_state_previous.keys[key].b_is_down;
}

int cx_input_was_key_repeated(enum cx_key key) {
	CX_ASSERT(key >= 0 && key < CX_KEY_MAX_, INPUT);
	return g_p_input_state_present->keys[key].repeat_count > 0;
}

int cx_input_was_key_pressed_or_repeated(enum cx_key key) {
	CX_ASSERT(key >= 0 && key < CX_KEY_MAX_, INPUT);
	return
		(g_p_input_state_present->keys[key].b_is_down && !g_input_state_previous.keys[key].b_is_down) ||
		g_p_input_state_present->keys[key].repeat_count > 0;
}

int cx_input_was_button_pressed(enum cx_button button) {
	CX_ASSERT(button >= 0 && button < CX_BUTTON_MAX_, INPUT);
	return g_p_input_state_present->buttons[button].b_is_down && !g_input_state_previous.buttons[button].b_is_down;
}

int cx_input_was_button_released(enum cx_button button) {
	CX_ASSERT(button >= 0 && button < CX_BUTTON_MAX_, INPUT);
	return !g_p_input_state_present->buttons[button].b_is_down && g_input_state_previous.buttons[button].b_is_down;
}

unsigned int cx_input_mods(void) {
	return g_p_input_state_present->mods;
}

int cx_input_is_text_buffer_empty(void) {
	return g_p_input_state_present->text_input_len == 0;
}

void cx_input_get_text_buffer(const char** pp_out, unsigned int* p_out_len) {
	*pp_out = g_p_input_state_present->text_input_buf;
	*p_out_len = g_p_input_state_present->text_input_len;
}
