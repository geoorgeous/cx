#ifndef _H__PLATFORM_WINDOW
#define _H__PLATFORM_WINDOW

#include "errors.h"
#include "keys.h"
#include "mouse_buttons.h"
#include "platform.h"

struct platform_window {
#if PLATFORM_WINDOWS
    HWND  _hwnd;
    HDC   _hdc;
    SHORT _mouse_pos[2];
    SHORT _mouse_pos_old[2];
#endif
    void(*_p_callback_on_created)(struct platform_window*, void*);
    void* _p_callback_on_created_user_ptr;
    void(*_p_callback_on_close)(struct platform_window*, void*);
    void* _p_callback_on_close_user_ptr;
    void(*_p_callback_on_focus_change)(struct platform_window*, void*, int);
    void* _p_callback_on_focus_change_user_ptr;
    void(*_p_callback_on_resize)(struct platform_window*, void*, int, int);
    void* _p_callback_on_resize_user_ptr;
    void(*_p_callback_on_key)(struct platform_window*, void*, enum key, int);
    void* _p_callback_on_key_user_ptr;
    void(*_p_callback_on_mouse_button)(struct platform_window*, void*, enum mouse_button, int);
    void* _p_callback_on_mouse_button_user_ptr;
    void(*_p_callback_on_mouse_move)(struct platform_window*, void*, int, int);
    void* _p_callback_on_mouse_move_user_ptr;
    void(*_p_callback_on_scroll)(struct platform_window*, void*, int);
    void* _p_callback_on_scroll_user_ptr;
    void(*_p_callback_on_char)(struct platform_window*, void*, int);
    void* _p_callback_on_char_user_ptr;
};

struct platform_window_create_options {
    int width;
    int height;
    const char* s_title;
};

enum error platform_window_create(int width, int height, const char* s_title, struct platform_window* p_platform_window, void(*p_callback_on_created)(struct platform_window*, void*), void* p_callback_on_created_user_ptr);
void       platform_window_destroy(struct platform_window* p_platform_window);
void       platform_window_poll(struct platform_window* p_platform_window);
int        platform_window_is_open(const struct platform_window* p_platform_window);
void       platform_window_size(const struct platform_window* p_platform_window, unsigned int* p_width, unsigned int* p_height);
void       platform_window_get_mouse_client_coords(const struct platform_window* p_platform_window, int* p_client_x, int* p_client_y);
void       platform_window_normalize_client_coords(const struct platform_window* p_platform_window, int client_x, int client_y, float* p_x, float* p_y);
void       platform_window_client_coords_to_ndc(const struct platform_window* p_platform_window, int client_x, int client_y, float* p_ndc_x, float* p_ndc_y);
void       platform_window_client_to_world_ray(const struct platform_window* p_platform_window, const float* p_camera_matrix, int client_x, int client_y, float* p_result);

void platform_window_set_on_close_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*), void* p_user_ptr);
void platform_window_set_on_focus_change_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int), void* p_user_ptr);
void platform_window_set_on_resize_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int, int), void* p_user_ptr);
void platform_window_set_on_key_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, enum key, int), void* p_user_ptr);
void platform_window_set_on_mouse_button_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, enum mouse_button, int), void* p_user_ptr);
void platform_window_set_on_mouse_move_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int, int), void* p_user_ptr);
void platform_window_set_on_scroll_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int), void* p_user_ptr);
void platform_window_set_on_char_callback(struct platform_window* p_platform_window, void(*p_callback)(struct platform_window*, void*, int), void* p_user_ptr);

#endif