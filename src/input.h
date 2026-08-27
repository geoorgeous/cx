#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#include "cx_keys.h"
#include "cx_mouse_buttons.h"

#define CX_LOG_CAT_INPUT "input"

enum input_mod {
	INPUT_MOD_ctrl  = (0x1 << 0),
	INPUT_MOD_shift = (0x1 << 1),
	INPUT_MOD_1     = (0x1 << 2),
	INPUT_MOD_2     = (0x1 << 3),
	INPUT_MOD_3     = (0x1 << 4),
	INPUT_MOD_4     = (0x1 << 5),
	INPUT_MOD_5     = (0x1 << 6),
};

enum input_event {
	INPUT_EVENT_key,
	INPUT_EVENT_mouse_button,
	INPUT_EVENT_mouse_move,
	INPUT_EVENT_scroll,
	INPUT_EVENT_char,
	INPUT_EVENT_MAX_
};

struct input_event_data_key {
	enum cx_key  key;
	int          b_is_down;
	unsigned int mods;
};

struct input_event_data_mouse_button {
	enum cx_mouse_button button;
	int                  b_is_down;
	int                  client_pos[2];
	unsigned int         mods;
};

struct input_event_data_mouse_move {
	int          delta_x;
	int          delta_y;
	unsigned int mods;
};

struct input_event_data_scroll {
	int delta_x;
	int delta_y;
};

struct input_event_data_char {
	uint32_t code;
};

void input_init(void);
int  input_frame_is_key_down(enum cx_key key);
int  input_frame_is_key_pressed(enum cx_key key);
int  input_frame_is_key_released(enum cx_key key);
int  input_frame_is_mouse_button_down(enum cx_mouse_button mouse_button);
int  input_frame_is_mouse_button_pressed(enum cx_mouse_button mouse_button);
int  input_frame_is_mouse_button_released(enum cx_mouse_button mouse_button);
void input_frame_mouse_delta(int* p_x, int* p_y);
void input_frame_scroll_delta(int* p_x, int* p_y);
void input_frame_reset(void);
void input_event_subscribe(enum input_event event, void(*p_event_callback)(const void*, void*), void* p_user_ptr);
void input_event_unsubscribe(enum input_event event, void(*p_event_callback)(const void*, void*));
void input_event_broadcast(enum input_event event, const void* p_event_data);

#endif
