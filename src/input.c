#include <stdlib.h>

#include "event.h"
#include "input.h"

struct key_state {
    int b_is_down;
};

struct mouse_button_state {
    int b_is_down;
};

struct input_frame_state {
    struct key_state          keys[KEY__MAX];
    struct mouse_button_state mouse_button[MOUSE_BUTTON__MAX];
    int                       mouse_delta[2];
    int                       scroll_delta[2];
};

static struct input {
    struct event             events[4];
    struct input_frame_state frame;
    struct input_frame_state frame_old;
} input;

void input_on_key(const void*, void*);
void input_on_mouse_button(const void*, void*);
void input_on_mouse_move(const void*, void*);
void input_on_scroll(const void*, void*);

void input_init(void) {
    input_event_subscribe(INPUT_EVENT_key, input_on_key, &input);
    input_event_subscribe(INPUT_EVENT_mouse_button, input_on_mouse_button, &input);
    input_event_subscribe(INPUT_EVENT_mouse_move, input_on_mouse_move, &input);
    input_event_subscribe(INPUT_EVENT_scroll, input_on_scroll, &input);
}

int input_frame_is_key_down(enum key key) {
    if (key < 0 || key >= KEY__MAX) {
        return 0;
    }

    return input.frame.keys[key].b_is_down;
}

int input_frame_is_mouse_button_down(enum mouse_button mouse_button) {
    if (mouse_button < 0 || mouse_button >= MOUSE_BUTTON__MAX) {
        return 0;
    }

    return input.frame.mouse_button[mouse_button].b_is_down;
}

void input_frame_mouse_delta(int* p_x, int* p_y) {
    *p_x = input.frame.mouse_delta[0];
    *p_y = input.frame.mouse_delta[1];
}

void input_frame_scroll_delta(int* p_x, int* p_y) {
    *p_x = input.frame.scroll_delta[0];
    *p_y = input.frame.scroll_delta[1];
}

void input_frame_reset(void) {
    input.frame_old = input.frame;
    input.frame.mouse_delta[0] = 0;
    input.frame.mouse_delta[1] = 0;
    input.frame.scroll_delta[0]  = 0;
    input.frame.scroll_delta[1]  = 0;
}

void input_event_subscribe(enum input_event event, void(*p_event_callback)(const void*, void*), void* p_user_ptr) {
    event_subscribe(&input.events[event], p_event_callback, p_user_ptr);
}

void input_event_unsubscribe(enum input_event event, void(*p_event_callback)(const void*, void*)) {
    event_unsubscribe(&input.events[event], p_event_callback);
}

void input_event_broadcast(enum input_event event, const void* p_event_data) {
    event_broadcast(&input.events[event], p_event_data);
}

void input_on_key(const void* p_e, void* p_user_ptr) {
    const struct input_event_data_key* p_key_event = p_e;
    struct input* p_input = p_user_ptr;

    if (p_key_event->key < 0 || p_key_event->key >= KEY__MAX) {
        return;
    }

    p_input->frame.keys[p_key_event->key].b_is_down = p_key_event->b_is_down;
}

void input_on_mouse_button(const void* p_e, void* p_user_ptr) {
    const struct input_event_data_mouse_button* p_mouse_button_event = p_e;
    struct input* p_input = p_user_ptr;

    if (p_mouse_button_event->button < 0 || p_mouse_button_event->button >= MOUSE_BUTTON__MAX) {
        return;
    }

    p_input->frame.mouse_button[p_mouse_button_event->button].b_is_down = p_mouse_button_event->b_is_down;
}

void input_on_mouse_move(const void* p_e, void* p_user_ptr) {
    const struct input_event_data_mouse_move* p_mouse_move_event = p_e;
    struct input* p_input = p_user_ptr;
    p_input->frame.mouse_delta[0] += p_mouse_move_event->delta_x;
    p_input->frame.mouse_delta[1] += p_mouse_move_event->delta_y;
}

void input_on_scroll(const void* p_e, void* p_user_ptr) {
    const struct input_event_data_scroll* p_scroll_event = p_e;
    struct input* p_input = p_user_ptr;
    p_input->frame.scroll_delta[0] += p_scroll_event->delta_x;
    p_input->frame.scroll_delta[1] += p_scroll_event->delta_y;
}