#include <string.h>

#include "cx_color.h"
#include "cx_dbg.h"
#include "cx_imgui.h"
#include "cx_input.h"
#include "cx_input_mods.h"
#include "cx_macro.h"
#include "cx_math.h"
#include "cx_text_edit.h"
#include "cx_text_mesher.h"

#define CX_IMGUI_ID_NONE 0

#define CX_IMGUI_DEFAULT_STYLE_COLOR_BACKGROUND   0x111111ff
#define CX_IMGUI_DEFAULT_STYLE_COLOR_FOREGROUND   0xffffffff
#define CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_A     0x333333ff
#define CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_B     0x555555ff
#define CX_IMGUI_DEFAULT_STYLE_PADDING_X          3.0f
#define CX_IMGUI_DEFAULT_STYLE_PADDING_Y          2.0f
#define CX_IMGUI_DEFAULT_STYLE_BORDER_SIZE        2.0f
#define CX_IMGUI_DEFAULT_STYLE_TEXT_INPUT_WIDTH   200.0f

struct cx_imgui_item {
	cx_imgui_id id;
	struct cx_imgui_rect rect;
};

struct cx_imgui_window {
	struct cx_imgui_rect rect;
	struct cx_imgui_vec2 content_size;
	struct cx_imgui_vec2 scroll;
	struct cx_imgui_vec2 scroll_max;
};

static cx_imgui_id cx_imgui_gen_id(void);

static void cx_imgui_item_begin(
	struct cx_imgui* p_ctx, const struct cx_imgui_vec2* p_desired_size, struct cx_imgui_item* p_out);

static void cx_imgui_text_measure(
	struct cx_imgui* p_ctx, const char* p_text, size_t text_len, struct cx_imgui_vec2* p_out);

static void cx_imgui_submit_quad(
	struct cx_imgui* p_ctx, const struct cx_imgui_rect* p_rect, const struct cx_color* p_color);

static void cx_imgui_submit_text(
	struct cx_imgui* p_ctx, const struct cx_imgui_vec2* p_pos, const char* p_str, size_t str_len);

static int cx_imgui_rect_test(const struct cx_imgui_rect* p_rect, const struct cx_imgui_vec2* p_vec2);

void cx_imgui_begin(struct cx_imgui* p_ctx) {
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_BACKGROUND, &p_ctx->theme.color_background);
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_FOREGROUND, &p_ctx->theme.color_foreground);
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_A, &p_ctx->theme.color_accent_a);
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_B, &p_ctx->theme.color_accent_b);
	p_ctx->theme.padding_x = CX_IMGUI_DEFAULT_STYLE_PADDING_X;
	p_ctx->theme.padding_y = CX_IMGUI_DEFAULT_STYLE_PADDING_Y;
	p_ctx->theme.border_size = CX_IMGUI_DEFAULT_STYLE_BORDER_SIZE;
	p_ctx->theme.text_input_width = CX_IMGUI_DEFAULT_STYLE_TEXT_INPUT_WIDTH;
}

void cx_imgui_end(struct cx_imgui* p_ctx) {
	// todo
}

void cx_imgui_separator(struct cx_imgui *p_ctx) {
	struct cx_imgui_vec2 size;
	size.x = cx_imgui_get_available_width(p_ctx);
	size.y = p_ctx->theme.border_size;

	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, &size, &item);

	cx_imgui_submit_quad(p_ctx, &item.rect, &p_ctx->theme.color_accent_b);
}

void cx_imgui_text(struct cx_imgui* p_ctx, const char* s_str) {
	const size_t len = strlen(s_str);

	struct cx_imgui_vec2 size;
	cx_imgui_text_measure(p_ctx, s_str, len, &size);
	
	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, &size, &item);

	cx_imgui_submit_text(p_ctx, &item.rect.position, s_str, len);
}

int cx_imgui_button(struct cx_imgui* p_ctx, const char* s_str) {
	const size_t len = strlen(s_str);

	struct cx_imgui_vec2 size;
	cx_imgui_text_measure(p_ctx, s_str, len, &size);
	size.x += p_ctx->theme.padding_x * 2;
	size.y += p_ctx->theme.padding_y * 2;
	
	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, &size, &item);

	int result = CX_FALSE;
	
	if (p_ctx->active == item.id) {
		if (!cx_input_is_button_down(CX_BUTTON_mouse_left)) {
			result = p_ctx->hot == item.id;
			p_ctx->active = CX_IMGUI_ID_NONE;
		}
	} else if (p_ctx->hot == item.id && cx_input_was_button_pressed(CX_BUTTON_mouse_left)) {
		p_ctx->active = item.id;
	}

	int mouse_x, mouse_y;
	cx_input_mouse_position(&mouse_x, &mouse_y);

	struct cx_imgui_vec2 mouse_pos;
	mouse_pos.x = (float)mouse_x;
	mouse_pos.y = (float)mouse_y;

	if (cx_imgui_rect_test(&item.rect, &mouse_pos)) {
		p_ctx->hot = item.id;
	}

	// todo: change color depending on hot/active state
	
	cx_imgui_submit_quad(p_ctx, &item.rect, &p_ctx->theme.color_accent_a);

	struct cx_imgui_vec2 text_pos;
	text_pos.x = item.rect.position.x + p_ctx->theme.padding_x;
	text_pos.y = item.rect.position.y + p_ctx->theme.padding_y;

	cx_imgui_submit_text(p_ctx, &text_pos, s_str, len);

	return result;
}

int cx_imgui_text_input(struct cx_imgui* p_ctx, char* p_buf, size_t buf_len) {
	CX_ASSERT(buf_len > 0, IMGUI);

	const size_t len = strlen(p_buf);

	struct cx_imgui_vec2 size;
	cx_imgui_text_measure(p_ctx, "W", 0, &size);
	size.x = (p_ctx->theme.border_size + p_ctx->theme.padding_x) * 2 + p_ctx->theme.text_input_width;
	size.y = (p_ctx->theme.border_size + p_ctx->theme.padding_y) * 2 + size.y;
	
	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, &size, &item);

	int result = CX_FALSE;
	
	if (p_ctx->active == item.id) {
		struct cx_text_edit text_edit;
		text_edit.p_buf = p_buf;
		text_edit.buf_size = buf_len;
		text_edit.len = len;
		text_edit.cursor_pos = p_ctx->text_input_cursor_pos;

		if (cx_input_was_key_pressed_or_repeated(CX_KEY_left)) {
			if (cx_input_mods() & CX_INPUT_MOD_shift) {
				cx_text_edit_cursor_prev_word(&text_edit);
			} else {
				cx_text_edit_cursor_offset(&text_edit, -1);
			}
		}

		if (cx_input_was_key_pressed_or_repeated(CX_KEY_right)) {
			if (cx_input_mods() & CX_INPUT_MOD_shift) {
				cx_text_edit_cursor_next_word(&text_edit);
			} else {
				cx_text_edit_cursor_offset(&text_edit, 1);
			}
		}

		if (cx_input_was_key_pressed_or_repeated(CX_KEY_backspace)) {
			cx_text_edit_delete(&text_edit, -1);
		}

		if (cx_input_was_key_pressed_or_repeated(CX_KEY_delete)) {
			cx_text_edit_delete(&text_edit, 1);
		}

		if (cx_input_was_key_pressed_or_repeated(CX_KEY_home)) {
			cx_text_edit_cursor_set(&text_edit, 0);
		}

		if (cx_input_was_key_pressed_or_repeated(CX_KEY_end)) {
			cx_text_edit_cursor_set(&text_edit, text_edit.len);
		}

		if (!cx_input_is_text_buffer_empty()) {
			const char* p_input_text_buf;
			unsigned int input_text_len;
			cx_input_get_text_buffer(&p_input_text_buf, &input_text_len);

			cx_text_edit_insert(&text_edit, p_input_text_buf, input_text_len);

			result = CX_TRUE;
		}

		p_ctx->text_input_cursor_pos = text_edit.cursor_pos;

		if (!cx_input_is_button_down(CX_BUTTON_mouse_left)) {
			result = p_ctx->hot == item.id;
			p_ctx->active = CX_IMGUI_ID_NONE;
		}
	} else if (p_ctx->hot == item.id && cx_input_was_button_pressed(CX_BUTTON_mouse_left)) {
		p_ctx->active = item.id;
		p_ctx->text_input_cursor_pos = 0;
	}

	int mouse_x, mouse_y;
	cx_input_mouse_position(&mouse_x, &mouse_y);

	struct cx_imgui_vec2 mouse_pos;
	mouse_pos.x = (float)mouse_x;
	mouse_pos.y = (float)mouse_y;

	if (cx_imgui_rect_test(&item.rect, &mouse_pos)) {
		p_ctx->hot = item.id;
	}
	
	cx_imgui_submit_quad(p_ctx, &item.rect, &p_ctx->theme.color_accent_a);

	struct cx_imgui_rect background_rect;
	background_rect.position.x = item.rect.position.x + p_ctx->theme.border_size;
	background_rect.position.y = item.rect.position.y + p_ctx->theme.border_size;
	background_rect.size.x = item.rect.size.x - p_ctx->theme.border_size * 2;
	background_rect.size.y = item.rect.size.y - p_ctx->theme.border_size * 2;
	cx_imgui_submit_quad(p_ctx, &background_rect, &p_ctx->theme.color_background);

	struct cx_imgui_vec2 text_pos;
	text_pos.x = background_rect.position.x + p_ctx->theme.padding_x;
	text_pos.y = background_rect.position.y + p_ctx->theme.padding_y;
	cx_imgui_submit_text(p_ctx, &text_pos, p_buf, 0);

	struct cx_imgui_rect cursor_rect;

	cx_imgui_text_measure(p_ctx, p_buf, p_ctx->text_input_cursor_pos, &cursor_rect.size);

	cursor_rect.position.x = text_pos.x + cursor_rect.size.x;
	cursor_rect.position.y = text_pos.y;
	cursor_rect.size.x = 1.0f;

	cx_imgui_submit_quad(p_ctx, &cursor_rect, &p_ctx->theme.color_foreground);

	return result;
}

void cx_imgui_same_line(struct cx_imgui* p_ctx) {
	p_ctx->layout.b_same_line = CX_TRUE;
}

float cx_imgui_get_available_width(struct cx_imgui* p_ctx) {
	// todo
	return 0.0f;
}

void cx_imgui_set_next_width(struct cx_imgui* p_ctx, float next_width) {
	p_ctx->layout.next_width = next_width;
}

cx_imgui_id cx_imgui_gen_id(void) {
	// todo
	return 0;
}

void cx_imgui_item_begin(
	struct cx_imgui* p_ctx, const struct cx_imgui_vec2* p_desired_size, struct cx_imgui_item* p_out) {
	
	*p_out = (struct cx_imgui_item){0};

	p_out->id = cx_imgui_gen_id();
	p_out->rect.position = p_ctx->layout.cursor;
	p_out->rect.size = *p_desired_size;

	if (p_ctx->layout.next_width >= 0.0f) {
		p_out->rect.size.x = p_ctx->layout.next_width;
		p_ctx->layout.next_width = -1.0f;
	}

	p_ctx->layout.line_height = CX_M_MAX(p_ctx->layout.line_height, p_out->rect.size.y);

	if (p_ctx->layout.b_same_line) {
		p_ctx->layout.cursor.x += p_out->rect.size.x;
	} else {
		p_ctx->layout.cursor.x = 0;
		p_ctx->layout.cursor.y += p_ctx->layout.line_height;
		p_ctx->layout.line_height = 0;
	}

	p_ctx->layout.b_same_line = CX_FALSE;
}

// todo: wrapping max-width and line spacing
void cx_imgui_text_measure(
	struct cx_imgui* p_ctx, const char* p_text, size_t text_len, struct cx_imgui_vec2* p_out) {

	cx_text_mesher_measure(p_text, text_len, p_ctx->theme.p_font, 1.0f, &p_out->x, &p_out->y);
}

void cx_imgui_submit_quad(
	struct cx_imgui* p_ctx, const struct cx_imgui_rect* p_rect, const struct cx_color* p_color) {

	// todo
}

void cx_imgui_submit_text(
	struct cx_imgui* p_ctx, const struct cx_imgui_vec2* p_pos, const char* p_str, size_t str_len) {
	
	// todo
}

int cx_imgui_rect_test(const struct cx_imgui_rect* p_rect, const struct cx_imgui_vec2* p_vec2) {
	return
		p_vec2->x >= p_rect->position.x &&
		p_vec2->x <= p_rect->position.x + p_rect->size.x &&
		p_vec2->y >= p_rect->position.y &&
		p_vec2->y <= p_rect->position.y + p_rect->size.y;
}
