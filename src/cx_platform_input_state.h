#ifndef CX_PLATFORM_INPUT_STATE_H
#define CX_PLATFORM_INPUT_STATE_H

#include "cx_buttons.h"
#include "cx_keys.h"

struct cx_platform_input_key_state {
	int b_is_down;
	unsigned int repeat_count;
};

struct cx_platform_input_button_state {
	int b_is_down;
};

struct cx_platform_input_state {
	struct cx_platform_input_key_state keys[CX_KEY_MAX_];
	struct cx_platform_input_button_state buttons[CX_BUTTON_MAX_];
	int mouse_x;
	int mouse_y;
	int scroll_accum_x;
	int scroll_accum_y;
	unsigned int mods;
	char text_input_buf[256];
	unsigned int text_input_len;
};

#endif
