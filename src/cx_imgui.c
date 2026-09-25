#include <string.h>

#include "cx_alloc.h"
#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_asset_defs.h"
#include "cx_dbg.h"
#include "cx_font.h"
#include "cx_gfx_texture.h"
#include "cx_image.h"
#include "cx_imgui.h"
#include "cx_input.h"
#include "cx_input_mods.h"
#include "cx_macro.h"
#include "cx_math.h"
#include "cx_mesh_gen.h"
#include "cx_shader.h"
#include "cx_str.h"
#include "cx_texture_atlas_layout.h"
#include "cx_text_edit.h"
#include "cx_text_mesher.h"
#include "matrix.h"
#include "vector.h"

#define CX_IMGUI_ID_NONE 0

#define CX_IMGUI_DEFAULT_STYLE_COLOR_BACKGROUND   0x343f44ff
#define CX_IMGUI_DEFAULT_STYLE_COLOR_FOREGROUND   0xe4dcccff
#define CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_1     0x64734cff
#define CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_2     0xa7c080ff
#define CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_3     0xc1d2a6ff
#define CX_IMGUI_DEFAULT_STYLE_PADDING_X          6.0f
#define CX_IMGUI_DEFAULT_STYLE_PADDING_Y          4.0f
#define CX_IMGUI_DEFAULT_STYLE_BORDER_SIZE        2.0f
#define CX_IMGUI_DEFAULT_STYLE_TEXT_INPUT_WIDTH   200.0f
#define CX_IMGUI_DEFAULT_STYLE_DROPDOWN_WIDTH     200.0f

#define CX_IMGUI_COLOR_BACKGROUND 0
#define CX_IMGUI_COLOR_FOREGROUND 1
#define CX_IMGUI_COLOR_ACCENT_1   2
#define CX_IMGUI_COLOR_ACCENT_2   3
#define CX_IMGUI_COLOR_ACCENT_3   4

#define CX_IMGUI_SEPERATOR_COLOR CX_IMGUI_COLOR_ACCENT_2

#define CX_IMGUI_BUTTON_COLOR CX_IMGUI_COLOR_ACCENT_1
#define CX_IMGUI_BUTTON_COLOR_HOT CX_IMGUI_COLOR_ACCENT_3
#define CX_IMGUI_BUTTON_COLOR_ACTIVE CX_IMGUI_COLOR_ACCENT_2

#define CX_IMGUI_TEXT_INPUT_BORDER_COLOR CX_IMGUI_COLOR_ACCENT_1
#define CX_IMGUI_TEXT_INPUT_BORDER_COLOR_HOT CX_IMGUI_COLOR_ACCENT_3
#define CX_IMGUI_TEXT_INPUT_BORDER_COLOR_ACTIVE CX_IMGUI_COLOR_ACCENT_2
#define CX_IMGUI_TEXT_INPUT_BACKGROUND_COLOR CX_IMGUI_COLOR_BACKGROUND
#define CX_IMGUI_TEXT_INPUT_CURSOR_COLOR CX_IMGUI_COLOR_FOREGROUND

static struct {
	struct cx_asset_ref asset_ref_default_font;
	struct cx_asset_ref asset_ref_quad_shader;
	struct cx_asset_ref asset_ref_text_shader;
	struct cx_render_pipeline pipeline_quad;
	struct cx_render_pipeline pipeline_text;
	struct cx_gfx_mesh mesh_quad;
	struct cx_texture_atlas_entry default_font_glyph_atlas_layout_entries[256];
	struct cx_texture_atlas_layout default_font_glyph_atlas_layout;
	struct cx_gfx_texture default_font_glyph_atlas_texture;
	struct cx_font_render_data default_font_render_data;
} g_shared_resources;

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

static void cx_imgui_init_shared_resources(void);

static cx_imgui_id cx_imgui_gen_id(const char* s_str);

static void cx_imgui_item_begin(
	struct cx_imgui* p_ctx, const char* s_id, const struct cx_imgui_vec2* p_desired_size, struct cx_imgui_item* p_out);

static void cx_imgui_text_measure(
	struct cx_imgui* p_ctx, const char* p_text, size_t text_len, struct cx_imgui_vec2* p_out);

static void cx_imgui_submit_quad(
	struct cx_imgui* p_ctx, const struct cx_imgui_rect* p_rect, const struct cx_color* p_color);

static void cx_imgui_submit_text(
	struct cx_imgui* p_ctx, const struct cx_imgui_vec2* p_pos, const char* p_str, size_t str_len);

static int cx_imgui_rect_test(const struct cx_imgui_rect* p_rect, const struct cx_imgui_vec2* p_vec2);

void cx_imgui_begin(struct cx_imgui* p_ctx, uint16_t width, uint16_t height) {
	cx_imgui_init_shared_resources();

	p_ctx->layout = (struct cx_imgui_layout) {0};

	p_ctx->num_quad_draw_commands = 0;
	p_ctx->num_text_draw_commands = 0;

	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_BACKGROUND, &p_ctx->theme.colors[CX_IMGUI_COLOR_BACKGROUND]);
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_FOREGROUND, &p_ctx->theme.colors[CX_IMGUI_COLOR_FOREGROUND]);
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_1, &p_ctx->theme.colors[CX_IMGUI_COLOR_ACCENT_1]);
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_2, &p_ctx->theme.colors[CX_IMGUI_COLOR_ACCENT_2]);
	cx_color_f32_from_u32(CX_IMGUI_DEFAULT_STYLE_COLOR_ACCENT_3, &p_ctx->theme.colors[CX_IMGUI_COLOR_ACCENT_3]);
	p_ctx->theme.padding_x = CX_IMGUI_DEFAULT_STYLE_PADDING_X;
	p_ctx->theme.padding_y = CX_IMGUI_DEFAULT_STYLE_PADDING_Y;
	p_ctx->theme.border_size = CX_IMGUI_DEFAULT_STYLE_BORDER_SIZE;
	p_ctx->theme.text_input_width = CX_IMGUI_DEFAULT_STYLE_TEXT_INPUT_WIDTH;
	p_ctx->theme.dropdown_width = CX_IMGUI_DEFAULT_STYLE_DROPDOWN_WIDTH;
	p_ctx->theme.p_font = &g_shared_resources.default_font_render_data;

	p_ctx->width = width;
	p_ctx->height = height;

	p_ctx->hot = CX_IMGUI_ID_NONE;
}

void cx_imgui_end(struct cx_imgui* p_ctx) {
	// todo
	
}

void cx_imgui_separator(struct cx_imgui *p_ctx) {
	struct cx_imgui_vec2 size;
	size.x = cx_imgui_get_available_width(p_ctx);
	size.y = p_ctx->theme.border_size;

	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, CX_NULL, &size, &item);

	cx_imgui_submit_quad(p_ctx, &item.rect, &p_ctx->theme.colors[CX_IMGUI_SEPERATOR_COLOR]);
}

void cx_imgui_text(struct cx_imgui* p_ctx, const char* s_str) {
	const size_t len = strlen(s_str);

	struct cx_imgui_vec2 size;
	cx_imgui_text_measure(p_ctx, s_str, len, &size);
	
	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, CX_NULL, &size, &item);

	cx_imgui_submit_text(p_ctx, &item.rect.position, s_str, len);
}

int cx_imgui_button(struct cx_imgui* p_ctx, const char* s_id, const char* s_str) {
	const size_t len = strlen(s_str);

	struct cx_imgui_vec2 text_size;
	cx_imgui_text_measure(p_ctx, s_str, len, &text_size);

	struct cx_imgui_vec2 desired_size;
	desired_size.x = text_size.x + p_ctx->theme.padding_x * 2;
	desired_size.y = text_size.y + p_ctx->theme.padding_y * 2;

	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, s_id, &desired_size, &item);

	int result = CX_FALSE;
	
	int mouse_x, mouse_y;
	cx_input_mouse_position(&mouse_x, &mouse_y);

	struct cx_imgui_vec2 mouse_pos;
	mouse_pos.x = (float)mouse_x;
	mouse_pos.y = (float)mouse_y;

	if (cx_imgui_rect_test(&item.rect, &mouse_pos)) {
		p_ctx->hot = item.id;
	}

	if (p_ctx->active == item.id) {
		if (!cx_input_is_button_down(CX_BUTTON_mouse_left)) {
			result = p_ctx->hot == item.id;
			p_ctx->active = CX_IMGUI_ID_NONE;
		}
	} else if (p_ctx->hot == item.id && cx_input_was_button_pressed(CX_BUTTON_mouse_left)) {
		p_ctx->active = item.id;
	}

	const struct cx_color* p_background_color =
		p_ctx->active == item.id ? &p_ctx->theme.colors[CX_IMGUI_BUTTON_COLOR_ACTIVE] :
		p_ctx->hot == item.id ? &p_ctx->theme.colors[CX_IMGUI_BUTTON_COLOR_HOT] :
		&p_ctx->theme.colors[CX_IMGUI_BUTTON_COLOR];
	
	cx_imgui_submit_quad(p_ctx, &item.rect, p_background_color);

	struct cx_imgui_vec2 text_pos;
	text_pos.x = item.rect.position.x;
	text_pos.y = item.rect.position.y;
	text_pos.x += (item.rect.size.x - text_size.x) * 0.5f;
	text_pos.y += (item.rect.size.y + text_size.y) * 0.5f - (float)p_ctx->theme.p_font->p_font->descent_;

	cx_imgui_submit_text(p_ctx, &text_pos, s_str, len);

	return result;
}

int cx_imgui_text_input(struct cx_imgui* p_ctx, const char* s_id, char* p_buf, size_t buf_len) {
	CX_ASSERT(buf_len > 0, IMGUI);

	// todo:
	// click to navigate cursor
	// make sure text is positioned so cursor is visible
	//   when the cursor position changes make sure that if it goes out of range then reposition text 

	struct cx_imgui_vec2 text_size;
	cx_imgui_text_measure(p_ctx, "|", 0, &text_size);

	struct cx_imgui_vec2 desired_size;
	desired_size.x = (p_ctx->theme.border_size + p_ctx->theme.padding_x) * 2 + p_ctx->theme.text_input_width;
	desired_size.y = (p_ctx->theme.border_size + p_ctx->theme.padding_y) * 2 + text_size.y;
	
	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, s_id, &desired_size, &item);

	int mouse_x, mouse_y;
	cx_input_mouse_position(&mouse_x, &mouse_y);

	struct cx_imgui_vec2 mouse_pos;
	mouse_pos.x = (float)mouse_x;
	mouse_pos.y = (float)mouse_y;

	if (cx_imgui_rect_test(&item.rect, &mouse_pos)) {
		p_ctx->hot = item.id;
	}
	
	int result = CX_FALSE;
	
	struct cx_text_edit text_edit;
	text_edit.p_buf = p_buf;
	text_edit.buf_size = buf_len;
	text_edit.len = strlen(p_buf);
	text_edit.cursor_pos = p_ctx->text_input_cursor_pos;

	if (p_ctx->active == item.id) {
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

			while (input_text_len--) {
				if (isprint(*p_input_text_buf)) {
					cx_text_edit_insert(&text_edit, p_input_text_buf, 1);
				}
				p_input_text_buf++;
			}
		}

		p_ctx->text_input_cursor_pos = text_edit.cursor_pos;

		if (p_ctx->hot != item.id && cx_input_was_button_pressed(CX_BUTTON_mouse_left)) {
			p_ctx->active = CX_IMGUI_ID_NONE;
			result = CX_TRUE;
		}
		
	} else if (p_ctx->hot == item.id && cx_input_was_button_pressed(CX_BUTTON_mouse_left)) {
		p_ctx->active = item.id;
		p_ctx->text_input_cursor_pos = 0;
	}

	const int border_color =
		p_ctx->active == item.id ? CX_IMGUI_TEXT_INPUT_BORDER_COLOR_ACTIVE :
		p_ctx->hot == item.id ? CX_IMGUI_TEXT_INPUT_BORDER_COLOR_HOT :
		CX_IMGUI_TEXT_INPUT_BORDER_COLOR;

	cx_imgui_submit_quad(p_ctx, &item.rect, &p_ctx->theme.colors[border_color]);

	struct cx_imgui_rect background_rect;
	background_rect.position.x = item.rect.position.x + p_ctx->theme.border_size;
	background_rect.position.y = item.rect.position.y + p_ctx->theme.border_size;
	background_rect.size.x = item.rect.size.x - p_ctx->theme.border_size * 2;
	background_rect.size.y = item.rect.size.y - p_ctx->theme.border_size * 2;
	cx_imgui_submit_quad(p_ctx, &background_rect, &p_ctx->theme.colors[CX_IMGUI_TEXT_INPUT_BACKGROUND_COLOR]);

	p_ctx->layout.b_scissor = CX_TRUE;
	p_ctx->layout.scissor_region = background_rect;

	struct cx_imgui_vec2 text_pos;
	text_pos.x = background_rect.position.x + p_ctx->theme.padding_x;
	text_pos.y = background_rect.position.y;
	text_pos.y += (background_rect.size.y + text_size.y) * 0.5f - (float)p_ctx->theme.p_font->p_font->descent_;

	if (text_edit.len > 0) {
		cx_imgui_submit_text(p_ctx, &text_pos, text_edit.p_buf, 0);
	}

	if (p_ctx->active == item.id) {
		struct cx_imgui_rect cursor_rect;

		cx_imgui_text_measure(p_ctx, text_edit.p_buf, p_ctx->text_input_cursor_pos, &cursor_rect.size);

		cursor_rect.position.x = text_pos.x + cursor_rect.size.x;
		cursor_rect.position.y = text_pos.y - (text_size.y - (float)p_ctx->theme.p_font->p_font->descent_);
		cursor_rect.size.x = 2.0f;

		cx_imgui_submit_quad(p_ctx, &cursor_rect, &p_ctx->theme.colors[CX_IMGUI_TEXT_INPUT_CURSOR_COLOR]);
	}

	p_ctx->layout.b_scissor = CX_FALSE;

	return result;
}

int cx_imgui_checkbox(struct cx_imgui* p_ctx, const char* s_id, int* p_b_value) {
}

int cx_imgui_begin_dropdown(struct cx_imgui* p_ctx, const char* s_id, const char* s_str) {

	// todo: functions like a button, but when clicked, opens a (scrollable?) list of other buttons.
	// when one of the sub-buttons are clicked, the current value and text of the main button is updated
	
	struct cx_imgui_vec2 text_size;
	cx_imgui_text_measure(p_ctx, "|", 0, &text_size);

	struct cx_imgui_vec2 desired_size;
	desired_size.x = (p_ctx->theme.border_size + p_ctx->theme.padding_x) * 2 + p_ctx->theme.dropdown_width;
	desired_size.y = (p_ctx->theme.border_size + p_ctx->theme.padding_y) * 2 + text_size.y;

	struct cx_imgui_item item;
	cx_imgui_item_begin(p_ctx, s_id, &desired_size, &item);

	int result = CX_FALSE;
	
	int mouse_x, mouse_y;
	cx_input_mouse_position(&mouse_x, &mouse_y);

	struct cx_imgui_vec2 mouse_pos;
	mouse_pos.x = (float)mouse_x;
	mouse_pos.y = (float)mouse_y;

	if (cx_imgui_rect_test(&item.rect, &mouse_pos)) {
		p_ctx->hot = item.id;
	}

	if (p_ctx->hot == item.id && cx_input_was_button_pressed(CX_BUTTON_mouse_left)) {
		p_ctx->active = item.id;
	}

	const struct cx_color* p_background_color =
		p_ctx->active == item.id ? &p_ctx->theme.colors[CX_IMGUI_BUTTON_COLOR_ACTIVE] :
		p_ctx->hot == item.id ? &p_ctx->theme.colors[CX_IMGUI_BUTTON_COLOR_HOT] :
		&p_ctx->theme.colors[CX_IMGUI_BUTTON_COLOR];
	
	cx_imgui_submit_quad(p_ctx, &item.rect, p_background_color);

	struct cx_imgui_vec2 text_pos;
	text_pos.x = item.rect.position.x;
	text_pos.y = item.rect.position.y;
	text_pos.x += p_ctx->theme.padding_x;
	text_pos.y += (item.rect.size.y + text_size.y) * 0.5f - (float)p_ctx->theme.p_font->p_font->descent_;

	cx_imgui_submit_text(p_ctx, &text_pos, s_str, 0);

	return result;
}

void cx_imgui_end_dropdown(struct cx_imgui *p_imgui) {
}

int cx_imgui_dropwdown_item(struct cx_imgui *p_imgui, const char* s_id, const char *s_str) {
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

void cx_imgui_init_shared_resources(void) {
	if (cx_asset_ref_is_set(&g_shared_resources.asset_ref_quad_shader)) {
		return;
	}

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_FONT, "default_8x14", &g_shared_resources.asset_ref_default_font);

	struct cx_font* p_font = cx_asset_cache_acquire(&g_shared_resources.asset_ref_default_font);

	g_shared_resources.default_font_glyph_atlas_layout.p_entries =
		g_shared_resources.default_font_glyph_atlas_layout_entries;
	g_shared_resources.default_font_glyph_atlas_layout.num_entries =
		CX_ARRAY_LEN(g_shared_resources.default_font_glyph_atlas_layout_entries);

	struct cx_image font_atlas_image;
	cx_font_create_atlas(p_font, &font_atlas_image, &g_shared_resources.default_font_glyph_atlas_layout);

	cx_gfx_texture_create(
		&g_shared_resources.default_font_glyph_atlas_texture,
		font_atlas_image.width, font_atlas_image.height,
		CX_PIXEL_FORMAT_red);

	cx_gfx_texture_set_data(
		&g_shared_resources.default_font_glyph_atlas_texture,
		font_atlas_image.p_pixel_data,
		&font_atlas_image.pixel_data_format);

	CX_FREE(font_atlas_image.p_pixel_data);

	g_shared_resources.default_font_render_data.p_font = p_font;
	g_shared_resources.default_font_render_data.p_glyph_texture =
		&g_shared_resources.default_font_glyph_atlas_texture;
	g_shared_resources.default_font_render_data.p_glyph_atlas_layout =
		&g_shared_resources.default_font_glyph_atlas_layout;

	struct cx_shader* p_shader;

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_flat", &g_shared_resources.asset_ref_quad_shader);
	
	p_shader = cx_asset_cache_acquire(&g_shared_resources.asset_ref_quad_shader);

	cx_shader_load_device_program(p_shader);

	g_shared_resources.pipeline_quad.p_shader = p_shader;
	g_shared_resources.pipeline_quad.state.flags = CX_RENDER_PIPELINE_FLAG_blend_enabled;
	g_shared_resources.pipeline_quad.state.blend_src_func = CX_BLEND_FUNC_src_alpha;
	g_shared_resources.pipeline_quad.state.blend_dst_func = CX_BLEND_FUNC_one_minus_src_alpha;
	g_shared_resources.pipeline_quad.state.cull_mode = CX_CULL_MODE_none;

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_text", &g_shared_resources.asset_ref_text_shader);
	
	p_shader = cx_asset_cache_acquire(&g_shared_resources.asset_ref_text_shader);

	cx_shader_load_device_program(p_shader);

	g_shared_resources.pipeline_text.p_shader = p_shader;
	g_shared_resources.pipeline_text.state.flags = CX_RENDER_PIPELINE_FLAG_blend_enabled;
	g_shared_resources.pipeline_text.state.blend_src_func = CX_BLEND_FUNC_src_alpha;
	g_shared_resources.pipeline_text.state.blend_dst_func = CX_BLEND_FUNC_one_minus_src_alpha;
	g_shared_resources.pipeline_text.state.cull_mode = CX_CULL_MODE_none;

	struct cx_mesh_data quad_mesh_data;
	cx_mesh_gen_quad(0.5f, 0.5f, (const float[]){ 0, 0, -1.0f }, &quad_mesh_data);

	cx_gfx_mesh_create(&quad_mesh_data, CX_GFX_BUFFER_USAGE_static, &g_shared_resources.mesh_quad);

	cx_mesh_gen_free(&quad_mesh_data);
}

cx_imgui_id cx_imgui_gen_id(const char* s_str) {
	return s_str == CX_NULL ? 0 : cx_str_hash(s_str);
}

void cx_imgui_item_begin(
	struct cx_imgui* p_ctx, const char* s_id, const struct cx_imgui_vec2* p_desired_size, struct cx_imgui_item* p_out) {
	
	*p_out = (struct cx_imgui_item){0};

	p_out->id = cx_imgui_gen_id(s_id);
	p_out->rect.position = p_ctx->layout.cursor;
	p_out->rect.size = *p_desired_size;

	if (p_ctx->layout.next_width > 0.0f) {
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

	const uint16_t num_draw_commands = p_ctx->num_quad_draw_commands + p_ctx->num_text_draw_commands;

	struct cx_gfx_shader_program_input_block* p_draw_command_input_set_blocks =
		&p_ctx->input_blocks[p_ctx->num_quad_draw_commands * 2 + p_ctx->num_text_draw_commands];

	float* p_material_color = p_ctx->draw_command_colors[p_ctx->num_quad_draw_commands];
	vec_copy(4, p_color->rgba, p_material_color);

	p_draw_command_input_set_blocks[0].s_name = "blk_material_properties";
	p_draw_command_input_set_blocks[0].size = sizeof(float) * 4;
	p_draw_command_input_set_blocks[0].p_data = p_material_color;

	float* p_vertex_matrix = p_ctx->draw_command_vertex_matrices[num_draw_commands];
	matrix_make_ts(
		p_rect->position.x + p_rect->size.x * 0.5f,
		p_ctx->height - (p_rect->position.y + p_rect->size.y * 0.5f),
		0,
		p_rect->size.x,
		p_rect->size.y,
		1,
		p_vertex_matrix);

	p_draw_command_input_set_blocks[1].s_name = "blk_object";
	p_draw_command_input_set_blocks[1].size = sizeof(float) * 16;
	p_draw_command_input_set_blocks[1].p_data = p_vertex_matrix;

	struct cx_render_draw_command* p_draw_command = &p_ctx->draw_commands[num_draw_commands];

	p_draw_command->pipeline = g_shared_resources.pipeline_quad;
	p_draw_command->draw_input_set.p_blocks = p_draw_command_input_set_blocks;
	p_draw_command->draw_input_set.num_blocks = 2;
	p_draw_command->p_mesh = &g_shared_resources.mesh_quad;
	p_draw_command->b_scissor = p_ctx->layout.b_scissor;
	
	if (p_draw_command->b_scissor) {
		p_draw_command->scissor_x = (int16_t)p_ctx->layout.scissor_region.position.x;
		p_draw_command->scissor_y = 
			p_ctx->height - (uint16_t)(p_ctx->layout.scissor_region.position.y + p_ctx->layout.scissor_region.size.y);
		p_draw_command->scissor_width = (uint16_t)p_ctx->layout.scissor_region.size.x;
		p_draw_command->scissor_height = (uint16_t)p_ctx->layout.scissor_region.size.y;
	}

	p_ctx->num_quad_draw_commands++;
}

void cx_imgui_submit_text(
	struct cx_imgui* p_ctx, const struct cx_imgui_vec2* p_pos, const char* p_str, size_t str_len) {
	
	const uint16_t num_draw_commands = p_ctx->num_quad_draw_commands + p_ctx->num_text_draw_commands;

	struct cx_text_mesher_input text_mesher_input = {0};
	text_mesher_input.s_text = p_str;
	text_mesher_input.style.p_font_render_data = p_ctx->theme.p_font;
	text_mesher_input.style.color = p_ctx->theme.colors[CX_IMGUI_COLOR_FOREGROUND];
	text_mesher_input.style.scale = 1.0f;

	struct cx_text_mesher_output text_mesher_output;
	size_t num_text_mesher_outputs;
	cx_text_mesher_generate(&text_mesher_input, 1, &text_mesher_output, &num_text_mesher_outputs);

	struct cx_gfx_mesh* p_mesh = &p_ctx->text_meshes[p_ctx->num_text_draw_commands];
	
	if (p_mesh->num_elements_ == 0) {
		cx_gfx_mesh_create(&text_mesher_output.mesh_data, CX_GFX_BUFFER_USAGE_dynamic, p_mesh);
	} else {
		cx_gfx_mesh_update(p_mesh, &text_mesher_output.mesh_data);
	}

	cx_text_mesher_free(&text_mesher_output, 1);

	p_ctx->input_texture.s_name = "u_texture_albedo";
	p_ctx->input_texture.p_texture = text_mesher_input.style.p_font_render_data->p_glyph_texture;
	
	struct cx_gfx_shader_program_input_block* p_draw_command_input_set_blocks =
		&p_ctx->input_blocks[p_ctx->num_quad_draw_commands * 2 + p_ctx->num_text_draw_commands];

	float* p_vertex_matrix = p_ctx->draw_command_vertex_matrices[num_draw_commands];
	matrix_make_translation(p_pos->x, p_ctx->height - p_pos->y, 0, p_vertex_matrix);

	p_draw_command_input_set_blocks->s_name = "blk_object";
	p_draw_command_input_set_blocks->size = sizeof(float) * 16;
	p_draw_command_input_set_blocks->p_data = p_vertex_matrix;

	struct cx_render_draw_command* p_draw_command = &p_ctx->draw_commands[num_draw_commands];

	p_draw_command->pipeline = g_shared_resources.pipeline_text;
	p_draw_command->draw_input_set.p_textures = &p_ctx->input_texture;
	p_draw_command->draw_input_set.num_textures = 1;
	p_draw_command->draw_input_set.p_blocks = p_draw_command_input_set_blocks;
	p_draw_command->draw_input_set.num_blocks = 1;
	p_draw_command->p_mesh = p_mesh;
	p_draw_command->b_scissor = p_ctx->layout.b_scissor;

	if (p_draw_command->b_scissor) {
		p_draw_command->scissor_x = (int16_t)p_ctx->layout.scissor_region.position.x;
		p_draw_command->scissor_y =
			p_ctx->height - (uint16_t)(p_ctx->layout.scissor_region.position.y + p_ctx->layout.scissor_region.size.y);
		p_draw_command->scissor_width = (uint16_t)p_ctx->layout.scissor_region.size.x;
		p_draw_command->scissor_height = (uint16_t)p_ctx->layout.scissor_region.size.y;
	}

	p_ctx->num_text_draw_commands++;
}

int cx_imgui_rect_test(const struct cx_imgui_rect* p_rect, const struct cx_imgui_vec2* p_vec2) {
	return
		p_vec2->x >= p_rect->position.x &&
		p_vec2->x <= p_rect->position.x + p_rect->size.x &&
		p_vec2->y >= p_rect->position.y &&
		p_vec2->y <= p_rect->position.y + p_rect->size.y;
}
