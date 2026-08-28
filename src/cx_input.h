#ifndef CX_INPUT_H
#define CX_INPUT_H

#include <stdint.h>

#include "cx_buttons.h"
#include "cx_keys.h"

#define CX_LOG_CAT_INPUT "input"

struct platform_window;

void cx_input_sample(const struct platform_window* p_window);
int cx_input_is_key_down(enum cx_key key);
int cx_input_is_button_down(enum cx_button button);
void cx_input_mouse_position(int* p_out_x, int* p_out_y);
void cx_input_mouse_delta(int* p_out_x, int* p_out_y);
void cx_input_scroll_accum(int* p_out_x, int* p_out_y);
int cx_input_was_key_pressed(enum cx_key key);
int cx_input_was_key_released(enum cx_key key);
int cx_input_was_key_repeated(enum cx_key key);
int cx_input_was_key_pressed_or_repeated(enum cx_key key);
int cx_input_was_button_pressed(enum cx_button button);
int cx_input_was_button_released(enum cx_button button);
unsigned int cx_input_mods(void);
int cx_input_is_text_buffer_empty(void);
void cx_input_get_text_buffer(const char** pp_out, unsigned int* p_out_len);

#endif
