#ifndef CX_ED_UI_H
#define CX_ED_UI_H

/*
 * Buttons
 *   
 *   When you click a button, call callback
 *
 * Images
 *
 * Textbox
 *
 *   You can click on them, and change the textbox content.
 *   When you finish editing the content, call callback
 *
 * Label
 *
 *   Displays some text
 *
 * Toggle box
 *
 *   When you click a togglebox, the toggle state changes. (flip a bool)
 *   When the toggle state changes, call callback
 *
 * Dropdown box
 *
 *   When you click a dropdown box, list all of the options.
 *   When an option is selected, change the current option, and call a callback
 *
 * Windows
 *
 *   An area for widgets to be drawn in
 *
 *
 * Client state: owned by caller/application
 * Interaction state: owned by UI
 * Widget state: reconstructed every frame
 */

#include <stddef.h>
#include <stdint.h>

#include "cx_buttons.h"
#include "cx_keys.h"
#include "cx_text_edit.h"

#define CX_ED_UI_MAX_ID_LEN 32
#define CX_ED_UI_MAX_WINDOWS 32
#define CX_ED_UI_MAX_LAYOUT_NODES 1024
#define CX_ED_UI_MAX_INTERACTIVE_WIDGETS CX_ED_UI_MAX_LAYOUT_NODES
#define CX_ED_UI_MAX_INTERACTIONS CX_ED_UI_MAX_LAYOUT_NODES
#define CX_ED_UI_MAX_HITBOXES CX_ED_UI_MAX_INTERACTIONS
#define CX_ED_UI_MAX_DRAW_COMMANDS CX_ED_UI_MAX_LAYOUT_NODES

#define CX_LOG_CAT_UI "ui"

struct cx_color;
struct cx_font_render_data;
struct cx_gfx_texture;
struct cx_texture;
struct platform_window;

enum cx_ed_ui_layout_type {
	CX_ED_UI_LAYOUT_TYPE_row,
	CX_ED_UI_LAYOUT_TYPE_column,
	CX_ED_UI_LAYOUT_TYPE_stack
};

enum cx_ed_ui_alignment {
	CX_ED_UI_ALIGNMENT_start,
	CX_ED_UI_ALIGNMENT_center,
	CX_ED_UI_ALIGNMENT_end,
};

struct cx_ed_ui_layout_node {
	enum cx_ed_ui_layout_type type;
	enum cx_ed_ui_alignment alignment_main;
	enum cx_ed_ui_alignment alignment_cross;

	uint16_t layer;
	uint16_t order;

	int16_t position[2];
	
	uint16_t min_size[2];
	uint16_t max_size[2];
	
	uint16_t desired_size[2];
	uint16_t size[2];

	uint16_t parent;
	uint16_t first_child;
	uint16_t next;

	int b_clip;
	int16_t clip_pos[2];
	uint16_t clip_size[2];
};

struct cx_ed_ui_button {
	uint16_t draw_command_bg;
	uint16_t draw_command_fg;
};

struct cx_ed_ui_textbox {
	uint16_t draw_command_border;
	uint16_t draw_command_bg;
	uint16_t draw_command_fg;
	char* p_text_buf;
	size_t text_buf_len;
};

enum cx_ed_ui_interactive_widget_type {
	CX_ED_UI_INTERACTIVE_WIDGET_TYPE_button,
	CX_ED_UI_INTERACTIVE_WIDGET_TYPE_textbox
};

struct cx_ed_ui_interactive_widget_record {
	enum cx_ed_ui_interactive_widget_type type;
	union {
		struct cx_ed_ui_button button;
		struct cx_ed_ui_textbox textbox;
	} data;
};

typedef void(*cx_ed_ui_mouse_enter_cb_fn)(void* p_user_ptr);
typedef void(*cx_ed_ui_mouse_exit_cb_fn)(void* p_user_ptr);
typedef void(*cx_ed_ui_mouse_move_cb_fn)(int, int, void* p_user_ptr);
typedef void(*cx_ed_ui_mouse_button_cb_fn)(enum cx_button, int, void*);
typedef void(*cx_ed_ui_key_cb_fn)(enum cx_key, int, void*);
typedef void(*cx_ed_ui_click_cb_fn)(void*);
typedef void(*cx_ed_ui_text_input_cb_fn)(const char*, void*);
typedef void(*cx_ed_ui_focus_enter_cb_fn)(void*);
typedef void(*cx_ed_ui_focus_exit_cb_fn)(void*);

struct cx_ed_ui_interaction_callbacks {
	cx_ed_ui_mouse_enter_cb_fn f_mouse_enter_cb;
	cx_ed_ui_mouse_exit_cb_fn f_mouse_exit_cb;
	cx_ed_ui_mouse_move_cb_fn f_mouse_move_cb;
	cx_ed_ui_mouse_button_cb_fn f_mouse_button_cb;
	cx_ed_ui_key_cb_fn f_key_cb;
	cx_ed_ui_click_cb_fn f_click_cb;
	cx_ed_ui_text_input_cb_fn f_text_input_cb;
	cx_ed_ui_focus_enter_cb_fn f_focus_enter_cb;
	cx_ed_ui_focus_exit_cb_fn f_focus_exit_cb;
	void* p_user_ptr;
};

struct cx_ed_ui_hit_box {
	const struct cx_ed_ui_layout_node* p_layout;
	uint16_t interaction;
};

struct cx_ed_ui_draw_command_quad {
	const struct cx_gfx_texture* p_gfx_texture;
	float color[4];
};

struct cx_ed_ui_draw_command_text {
	const struct cx_font_render_data* p_font_render_data;
	const char* s_text;
	float color[4];
};

struct cx_ed_ui_draw_command {
	const struct cx_ed_ui_layout_node* p_layout;
	int b_is_quad;
	union {
		struct cx_ed_ui_draw_command_quad as_quad;
		struct cx_ed_ui_draw_command_text as_text;
	} details;
};

struct cx_ed_ui_persistent_state_pool_slot {
	char id[CX_ED_UI_MAX_ID_LEN];
	uint16_t inactive_frames;
	int b_is_occupied;
};

struct cx_ed_ui_persistent_state_pool {
	void* p_slots;
	size_t slot_size;
	uint16_t capacity;
	uint16_t count;
};

struct cx_ed_ui_window_state {
	struct cx_ed_ui_persistent_state_pool_slot pool_slot;
	int16_t position[2];
	uint16_t size[2];
	uint16_t layer;
};

struct cx_ed_ui_interaction_state {
	struct cx_ed_ui_persistent_state_pool_slot pool_slot;
	struct cx_ed_ui_layout_node* p_layout;
	struct cx_ed_ui_interaction_callbacks callbacks;
	uint16_t interactive_widget;
};

enum cx_ed_ui_input_event_type {
	CX_ED_UI_INPUT_EVENT_TYPE_mouse_button,
	CX_ED_UI_INPUT_EVENT_TYPE_keys,
	CX_ED_UI_INPUT_EVENT_TYPE_charcode
};

struct cx_ed_ui_input_event {
	enum cx_ed_ui_input_event_type type;
	union {
		struct {
			enum cx_button button;
			int b_is_down;
			unsigned int mods;
		} mouse_button;
		struct {
			enum cx_key key;
			int b_is_down;
			unsigned int mods;
		} key;
		struct {
			uint32_t code;
		} charcode;
	} data;
};

struct cx_ed_ui {
	struct cx_ed_ui_window_state windows[CX_ED_UI_MAX_WINDOWS];
	struct cx_ed_ui_persistent_state_pool window_pool;

	struct cx_ed_ui_interaction_state interactions[CX_ED_UI_MAX_INTERACTIONS];
	struct cx_ed_ui_persistent_state_pool interaction_pool;

	struct cx_ed_ui_layout_node layout_nodes[CX_ED_UI_MAX_LAYOUT_NODES];
	uint16_t num_layout_nodes;
	uint16_t current_node;

	struct cx_ed_ui_interactive_widget_record interactive_widgets[CX_ED_UI_MAX_INTERACTIVE_WIDGETS];
	uint16_t num_interactive_widgets;

	struct cx_ed_ui_hit_box hitboxes[CX_ED_UI_MAX_HITBOXES];
	uint16_t num_hitboxes;

	struct cx_ed_ui_draw_command draw_commands[CX_ED_UI_MAX_DRAW_COMMANDS];
	uint16_t num_draw_commands;

	uint16_t current_window;

	uint16_t interaction_hovered;
	uint16_t interaction_pressed[CX_BUTTON_MAX_];
	uint16_t interaction_focused;

	uint32_t canvas_width;
	uint32_t canvas_height;

	struct cx_ed_ui_input_event events[128];
	uint16_t num_events;

	struct cx_text_edit text_edit;

	int b_clip;
	int16_t clip_rect_pos[2];
	uint16_t clip_rect_size[2];
};

void cx_ed_ui_init(uint32_t canvas_width, uint32_t canvas_height, struct cx_ed_ui* p_out);

void cx_ed_ui_window_begin(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const char* s_name,
	int16_t left,
	int16_t top,
	uint16_t max_width,
	uint16_t max_height);
void cx_ed_ui_window_end(struct cx_ed_ui* p_ui);

void cx_ed_ui_row_begin(struct cx_ed_ui* p_ui, enum cx_ed_ui_alignment alignment);
void cx_ed_ui_row_end(struct cx_ed_ui* p_ui);
void cx_ed_ui_column_begin(struct cx_ed_ui* p_ui, enum cx_ed_ui_alignment alignment);
void cx_ed_ui_column_end(struct cx_ed_ui* p_ui);

void cx_ed_ui_image(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	struct cx_texture* p_texture,
	uint16_t width,
	uint16_t height,
	const struct cx_color* p_color);

void cx_ed_ui_label(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	const char* s_text,
	const struct cx_font_render_data* p_font_render_data,
	const struct cx_color* p_color);

void cx_ed_ui_button(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	const char* s_text);

void cx_ed_ui_textbox(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	char* p_text_buf,
	size_t text_buf_len);

/* todo: */
/* button */
/* textinput */
/* dropdown box */
/* checkbox */

void cx_ed_ui_end_frame(struct cx_ed_ui* p_ui, const struct platform_window* p_window);

void cx_ed_ui_draw(struct cx_ed_ui* p_ui);

#endif
