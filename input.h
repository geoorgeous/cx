#ifndef _H__INPUT
#define _H__INPUT

#include <stdint.h>

#include "keys.h"
#include "mouse_buttons.h"

enum input_event {
    INPUT_EVENT_key,
    INPUT_EVENT_mouse_button,
    INPUT_EVENT_mouse_move,
    INPUT_EVENT_scroll
};

struct input_event_data_key {
    enum key key;
    int      b_is_down;
};

struct input_event_data_mouse_button {
    enum mouse_button button;
    int               b_is_down;
    int               client_pos[2];
};

struct input_event_data_mouse_move {
    int delta_x;
    int delta_y;
};

struct input_event_data_scroll {
    int delta_x;
    int delta_y;
};

void input_init(void);
int  input_frame_is_key_down(enum key key);
int  input_frame_is_mouse_button_down(enum mouse_button mouse_button);
void input_frame_mouse_delta(int* p_x, int* p_y);
void input_frame_scroll_delta(int* p_x, int* p_y);
void input_frame_reset(void);
void input_event_subscribe(enum input_event event, void(*p_event_callback)(const void*, void*), void* p_user_ptr);
void input_event_unsubscribe(enum input_event event, void(*p_event_callback)(const void*, void*));
void input_event_broadcast(enum input_event event, const void* p_event_data);

#endif