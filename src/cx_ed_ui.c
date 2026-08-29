#include <stdlib.h>
#include <string.h>

#include "gl.h"

#include "cx_asset_cache.h"
#include "cx_color.h"
#include "cx_dbg.h"
#include "cx_ed_ui.h"
#include "cx_font.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"
#include "cx_gfx_texture.h"
#include "cx_image.h"
#include "cx_input.h"
#include "cx_io.h"
#include "cx_macro.h"
#include "cx_math.h"
#include "cx_platform_window.h"
#include "cx_str.h"
#include "cx_texture.h"
#include "cx_texture_atlas_layout.h"
#include "cx_text_mesher.h"
#include "matrix.h"

#define CX_ED_UI_MAX_INACTIVE_FRAMES 0

#define CX_ED_UI_LAYOUT_NODE_PADDING 5

#define CX_ED_UI_LAYOUT_NODE_NONE CX_ED_UI_MAX_LAYOUT_NODES
#define CX_ED_UI_WINDOW_NONE CX_ED_UI_MAX_WINDOWS
#define CX_ED_UI_INTERACTION_NONE CX_ED_UI_MAX_INTERACTIONS
#define CX_ED_UI_INTERACTIVE_WIDGET_NONE CX_ED_UI_MAX_INTERACTIVE_WIDGETS

#define CX_ED_UI_COLOR_BG         0.176470f, 0.207843f, 0.231372f
#define CX_ED_UI_COLOR_BG_GREEN   0.258823f, 0.313725f, 0.278431f
#define CX_ED_UI_COLOR_GREEN      0.654901f, 0.752941f, 0.501960f
#define CX_ED_UI_COLOR_RED_DARK   0.286274f, 0.231372f, 0.250980f
#define CX_ED_UI_COLOR_RED_BRIGHT 0.901960f, 0.494117f, 0.501960f
#define CX_ED_UI_COLOR_RED        0.972549f, 0.333333f, 0.321568f
#define CX_ED_UI_COLOR_FG         0.827450f, 0.776470f, 0.666666f
#define CX_ED_UI_WINDOW_ALPHA     0.666666f

static int cx_ed_ui_persistent_state_pool_try_get(
	struct cx_ed_ui_persistent_state_pool* p_pool,
	const char* s_id,
	uint16_t* p_out_slot);
static void cx_ed_ui_persistent_state_pool_remove_inactive(struct cx_ed_ui_persistent_state_pool* p_pool);

static void cx_ed_ui_layout_node_new(
	struct cx_ed_ui* p_ui,
	enum cx_ed_ui_layout_type type,
	enum cx_ed_ui_alignment alignment,
	uint16_t parent,
	const char* s_interaction_id,
	const struct cx_ed_ui_interaction_callbacks* p_interaction_callbacks);
static void cx_ed_ui_layout_node_begin(
	struct cx_ed_ui* p_ui, enum cx_ed_ui_layout_type type, enum cx_ed_ui_alignment alignment);
static void cx_ed_ui_layout_node_end(struct cx_ed_ui* p_ui);
static void cx_ed_ui_layout_node_resolve_child_positions(
	struct cx_ed_ui* p_ui, const struct cx_ed_ui_layout_node* p_node);
static void cx_ed_ui_submit_quad(
	struct cx_ed_ui* p_ui, uint16_t width, uint16_t height, const struct cx_ed_ui_draw_command_quad* p_quad);
static void cx_ed_ui_submit_text(
	struct cx_ed_ui* p_ui, const struct cx_ed_ui_draw_command_text* p_text);
static void cx_ed_ui_init_shared_resources(void);

static struct {
	struct cx_gfx_program program;
	struct cx_gfx_program_param_block program_pblk_camera;
	struct cx_gfx_program_param_buffer program_pbuf_camera;
	struct cx_gfx_program_opaque_param program_sampler2d;
	struct cx_gfx_texture default_texture;
	struct cx_gfx_program text_program;
	struct cx_gfx_program_param_block text_program_pblk_camera;
	struct cx_gfx_program_param_buffer text_program_pbuf_camera;
	struct cx_gfx_program_param_block text_program_pblk_object;
	struct cx_gfx_program_param_buffer text_program_pbuf_object;
	struct cx_gfx_program_opaque_param text_program_sampler2d;
	struct cx_asset_ref default_font_ref;
	struct cx_texture_atlas_entry default_font_glyph_atlas_layout_entries[CX_FONT_NUM_GLYPHS];
	struct cx_texture_atlas_layout default_font_glyph_atlas_layout;
	struct cx_gfx_texture default_font_glyph_atlas_texture;
	struct cx_font_render_data default_font_render_data;
} shared_resources;

void cx_ed_ui_init(uint32_t canvas_width, uint32_t canvas_height, struct cx_ed_ui* p_out) {
	cx_ed_ui_init_shared_resources();

	*p_out = (struct cx_ed_ui) {
		.window_pool = {
			.p_slots = p_out->windows,
			.slot_size = sizeof(*p_out->windows),
			.capacity = sizeof(p_out->windows) / sizeof(*p_out->windows)
		},
		.interaction_pool = {
			.p_slots = p_out->interactions,
			.slot_size = sizeof(*p_out->interactions),
			.capacity = sizeof(p_out->interactions) / sizeof(*p_out->interactions)
		},
		.current_window = CX_ED_UI_WINDOW_NONE,
		.current_node = CX_ED_UI_LAYOUT_NODE_NONE,
		.interaction_hovered = CX_ED_UI_INTERACTION_NONE,
		.interaction_focused = CX_ED_UI_INTERACTION_NONE,
		.canvas_width = canvas_width,
		.canvas_height = canvas_height
	};
}

void cx_ed_ui_window_begin(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const char* s_name,
	int16_t left,
	int16_t top,
	uint16_t max_width,
	uint16_t max_height) {

	CX_ASSERT(p_ui->current_window == CX_ED_UI_WINDOW_NONE, UI);

	uint16_t window;
	if (!cx_ed_ui_persistent_state_pool_try_get(&p_ui->window_pool, s_id, &window)) {
		p_ui->windows[window].position[0] = left;
		p_ui->windows[window].position[1] = top;
	}

	p_ui->current_window = window;

	const struct cx_ed_ui_window_state* p_window = &p_ui->windows[p_ui->current_window];

	// window root layout

	cx_ed_ui_layout_node_begin(p_ui, CX_ED_UI_LAYOUT_TYPE_column, CX_ED_UI_ALIGNMENT_start);

	struct cx_ed_ui_layout_node* p_window_layout = &p_ui->layout_nodes[p_ui->current_node];

	p_window_layout->position[0] = p_window->position[0];
	p_window_layout->position[1] = p_window->position[1];
	p_window_layout->max_size[0] = max_width;
	p_window_layout->max_size[1] = max_height;

	// background quad

	p_ui->draw_commands[p_ui->num_draw_commands] = (struct cx_ed_ui_draw_command) {
		.p_layout = p_window_layout,
		.b_is_quad = CX_TRUE,
		.details.as_quad = {
			.p_gfx_texture = &shared_resources.default_texture,
			.color = { CX_ED_UI_COLOR_BG, CX_ED_UI_WINDOW_ALPHA }
		}
	};

	p_ui->num_draw_commands++;
}

void cx_ed_ui_window_end(struct cx_ed_ui* p_ui) {
	CX_ASSERT(p_ui->current_window != CX_ED_UI_WINDOW_NONE, UI);

	cx_ed_ui_layout_node_end(p_ui);
}

void cx_ed_ui_layout_node_new(
	struct cx_ed_ui* p_ui,
	enum cx_ed_ui_layout_type type,
	enum cx_ed_ui_alignment alignment,
	uint16_t parent,
	const char* s_interaction_id,
	const struct cx_ed_ui_interaction_callbacks* p_interaction_callbacks) {

	const uint16_t node_id = p_ui->num_layout_nodes;

	struct cx_ed_ui_layout_node* p_node = &p_ui->layout_nodes[node_id];
	*p_node = (struct cx_ed_ui_layout_node) {
		.type = type,
		.alignment_cross = alignment,
		.layer = p_ui->windows[p_ui->current_window].layer,
		.order = p_ui->num_layout_nodes,
		.parent = parent,
		.first_child = CX_ED_UI_LAYOUT_NODE_NONE,
		.next = CX_ED_UI_LAYOUT_NODE_NONE
	};

	// Add to parent's child linked list

	if (p_node->parent != CX_ED_UI_LAYOUT_NODE_NONE) {
		struct cx_ed_ui_layout_node* p_parent = &p_ui->layout_nodes[p_node->parent];

		uint16_t* p_parent_last_child = &p_parent->first_child;
		while(*p_parent_last_child != CX_ED_UI_LAYOUT_NODE_NONE) {
			p_parent_last_child = &p_ui->layout_nodes[*p_parent_last_child].next;
		}

		*p_parent_last_child = node_id;
	}

	p_ui->num_layout_nodes++;

	if (p_interaction_callbacks) {
		CX_ASSERT(s_interaction_id != CX_NULL, UI);

		uint16_t interaction;
		if (!cx_ed_ui_persistent_state_pool_try_get(&p_ui->interaction_pool, s_interaction_id, &interaction)) {
			p_ui->interactions[interaction].callbacks = *p_interaction_callbacks;
			p_ui->interactions[interaction].interactive_widget = CX_ED_UI_INTERACTIVE_WIDGET_NONE;
		}

		p_ui->interactions[interaction].p_layout = p_node;

		struct cx_ed_ui_hit_box* p_hit_box = &p_ui->hitboxes[p_ui->num_hitboxes];
		*p_hit_box = (struct cx_ed_ui_hit_box) {
			.p_layout = p_node,
			.interaction = interaction
		};

		p_ui->num_hitboxes++;
	}
}

void cx_ed_ui_layout_node_begin(
	struct cx_ed_ui* p_ui, enum cx_ed_ui_layout_type type, enum cx_ed_ui_alignment alignment) {

	CX_ASSERT(p_ui->current_window != CX_ED_UI_WINDOW_NONE, UI);
	CX_ASSERT(p_ui->num_layout_nodes < CX_ED_UI_MAX_LAYOUT_NODES - 1, UI);

	cx_ed_ui_layout_node_new(p_ui, type, alignment, p_ui->current_node, CX_NULL, CX_NULL);

	p_ui->current_node = p_ui->num_layout_nodes - 1;
}

void cx_ed_ui_layout_node_end(struct cx_ed_ui* p_ui) {
	CX_ASSERT(p_ui->current_node != CX_ED_UI_LAYOUT_NODE_NONE, UI);
	
	struct cx_ed_ui_layout_node* p_node = &p_ui->layout_nodes[p_ui->current_node];

	CX_ASSERT(p_node->first_child != CX_ED_UI_LAYOUT_NODE_NONE, UI);
	CX_ASSERT(p_node->min_size[0] <= p_node->max_size[0], UI);
	CX_ASSERT(p_node->min_size[1] <= p_node->max_size[1], UI);
	
	int main_axis = p_node->type == CX_ED_UI_LAYOUT_TYPE_row ? 0 : 1;
	int cross_axis = p_node->type == CX_ED_UI_LAYOUT_TYPE_row ? 1 : 0;

	// resolve parent node's desired size

	uint16_t child;

	child = p_node->first_child;
	while (child != CX_ED_UI_LAYOUT_NODE_NONE) {
		struct cx_ed_ui_layout_node* p_child = &p_ui->layout_nodes[child];

		CX_ASSERT(p_child->desired_size[0] != 0, UI);
		CX_ASSERT(p_child->desired_size[1] != 0, UI);
		CX_ASSERT(p_child->parent == p_ui->current_node, UI);

		if (p_node->type == CX_ED_UI_LAYOUT_TYPE_stack) {
			const int32_t child_right = p_child->position[0] + p_child->desired_size[0];
			if (p_node->desired_size[0] < child_right) {
				p_node->desired_size[0] = (uint16_t)child_right;
			}
			
			const int32_t child_bottom = p_child->position[1] + p_child->desired_size[1];
			if (p_node->desired_size[1] < child_bottom) {
				p_node->desired_size[1] = (uint16_t)child_bottom;
			}
			
			p_child->size[main_axis] = p_child->desired_size[main_axis];
			p_child->size[cross_axis] = p_child->desired_size[cross_axis];

			child = p_child->next;
		} else {
			if (p_node->desired_size[cross_axis] < p_child->desired_size[cross_axis]) {
				p_node->desired_size[cross_axis] = p_child->desired_size[cross_axis];
			}

			p_node->desired_size[main_axis] += p_child->desired_size[main_axis];

			p_child->size[main_axis] = p_child->desired_size[main_axis];
			p_child->size[cross_axis] = p_child->desired_size[cross_axis];

			child = p_child->next;

			if (child != CX_ED_UI_LAYOUT_NODE_NONE) {
				p_node->desired_size[main_axis] += CX_ED_UI_LAYOUT_NODE_PADDING;
			}
		}

		uint16_t min;
		uint16_t max;

		min = p_child->min_size[0] == 0 ? p_child->desired_size[0] : p_child->min_size[0];
		max = p_child->max_size[0] == 0 ? p_child->desired_size[0] : p_child->max_size[0];
		p_child->size[0] = CX_M_CLAMP(min, max, p_child->desired_size[0]);

		min = p_child->min_size[1] == 0 ? p_child->desired_size[1] : p_child->min_size[1];
		max = p_child->max_size[1] == 0 ? p_child->desired_size[1] : p_child->max_size[1];
		p_child->size[1] = CX_M_CLAMP(min, max, p_child->desired_size[1]);
	}

	// apply parent node size constraints

	uint16_t min;
	uint16_t max;

	min = p_node->min_size[0] == 0 ? p_node->desired_size[0] : p_node->min_size[0];
	max = p_node->max_size[0] == 0 ? p_node->desired_size[0] : p_node->max_size[0];
	p_node->size[0] = CX_M_CLAMP(min, max, p_node->desired_size[0]);

	min = p_node->min_size[1] == 0 ? p_node->desired_size[1] : p_node->min_size[1];
	max = p_node->max_size[1] == 0 ? p_node->desired_size[1] : p_node->max_size[1];
	p_node->size[1] = CX_M_CLAMP(min, max, p_node->desired_size[1]);

	//CX_LOG_FMT(INFO, UI, "layout_node_end() p=%p, width=%u, height=%u\n",
	//	p_node, p_node->size[0], p_node->size[1]);

	// position children relative to parent node position

	int16_t main_axis_pos = 0;

	child = p_node->first_child;
	while (child != CX_ED_UI_LAYOUT_NODE_NONE) {
		struct cx_ed_ui_layout_node* p_child = &p_ui->layout_nodes[child];

		// position along main axis

		if (p_node->type != CX_ED_UI_LAYOUT_TYPE_stack) {
			p_child->position[main_axis] += main_axis_pos;
			main_axis_pos += p_child->size[main_axis] + CX_ED_UI_LAYOUT_NODE_PADDING;
		}
		
		// position along cross axis

		switch (p_node->alignment_cross) {
			case CX_ED_UI_ALIGNMENT_start: {
				p_child->position[cross_axis] += 0;
				break;
			}
			case CX_ED_UI_ALIGNMENT_center: {
				p_child->position[cross_axis] += (p_node->size[cross_axis] - p_child->size[cross_axis]) / 2;
				break;
			}
			case CX_ED_UI_ALIGNMENT_end: {
				p_child->position[cross_axis] += p_node->size[cross_axis] - p_child->size[cross_axis];
				break;
			}
		};

		child = p_child->next;
	}

	p_ui->current_node = p_node->parent;

	if (p_ui->current_node != CX_ED_UI_LAYOUT_NODE_NONE) {
		return;
	}

	if (p_node->b_clip) {
		p_node->clip_pos[0] = p_node->position[0];
		p_node->clip_pos[1] = p_node->position[1];
		p_node->clip_size[0] = p_node->size[0];
		p_node->clip_size[1] = p_node->size[1];
	}

	cx_ed_ui_layout_node_resolve_child_positions(p_ui, p_node);
}

static void cx_ed_ui_layout_node_resolve_child_positions(
	struct cx_ed_ui* p_ui, const struct cx_ed_ui_layout_node* p_node) {

	uint16_t child;

	child = p_node->first_child;
	while (child != CX_ED_UI_LAYOUT_NODE_NONE) {
		struct cx_ed_ui_layout_node* p_child = &p_ui->layout_nodes[child];

		p_child->position[0] += p_node->position[0];
		p_child->position[1] += p_node->position[1];

		if (p_node->b_clip) {
			if (p_child->b_clip) {
				// intersection clip
				const int32_t left = CX_M_MAX(p_node->clip_pos[0], p_child->position[0]);
				const int32_t top = CX_M_MAX(p_node->clip_pos[1], p_child->position[1]);
				const int32_t right = CX_M_MAX(p_node->clip_pos[0] + p_node->clip_size[0], p_child->position[0] + p_child->size[0]);
				const int32_t bottom = CX_M_MAX(p_node->clip_pos[1] + p_node->clip_size[1], p_child->position[1] + p_child->size[1]);

				p_child->clip_pos[0] = (int16_t)left;
				p_child->clip_pos[1] = (int16_t)top;
				p_child->clip_size[0] = (uint16_t)CX_M_MAX(0, right - left);
				p_child->clip_size[1] = (uint16_t)CX_M_MAX(0, bottom - top);
			} else {
				p_child->b_clip = CX_TRUE;
				p_child->clip_pos[0] = p_node->clip_pos[0];
				p_child->clip_pos[1] = p_node->clip_pos[1];
				p_child->clip_size[0] = p_node->clip_size[0];
				p_child->clip_size[1] = p_node->clip_size[1];
			}
		} else if (p_child->b_clip) {
			p_child->clip_pos[0] = p_child->position[0];
			p_child->clip_pos[1] = p_child->position[1];
			p_child->clip_size[0] = p_child->size[0];
			p_child->clip_size[1] = p_child->size[1];
		}

		//CX_LOG_FMT(INFO, UI, "layout_node_resolve_child_positions(): child.pos={ %d, %d }\n",
			//p_child->position[0], p_child->position[1]);

		cx_ed_ui_layout_node_resolve_child_positions(p_ui, p_child);

		child = p_child->next;
	}
}

void cx_ed_ui_submit_quad(
	struct cx_ed_ui* p_ui, uint16_t width, uint16_t height, const struct cx_ed_ui_draw_command_quad* p_quad) {

	CX_ASSERT(width != 0 && height != 0, UI);

	struct cx_ed_ui_layout_node* p_node = &p_ui->layout_nodes[p_ui->num_layout_nodes - 1];
	p_node->desired_size[0] = width;
	p_node->desired_size[1] = height;

	p_ui->draw_commands[p_ui->num_draw_commands] = (struct cx_ed_ui_draw_command) {
		.p_layout = p_node,
		.b_is_quad = CX_TRUE,
		.details.as_quad = *p_quad
	};

	p_ui->num_draw_commands++;
}

void cx_ed_ui_submit_text(
	struct cx_ed_ui* p_ui, const struct cx_ed_ui_draw_command_text* p_text) {

	float text_width;
	float text_height;
	cx_text_mesher_measure(
		p_text->s_text, strlen(p_text->s_text), p_text->p_font_render_data, 1.0f, &text_width, &text_height);

	CX_ASSERT(!FLT_ISZERO(text_width) && !FLT_ISZERO(text_height), UI);

	struct cx_ed_ui_layout_node* p_node = &p_ui->layout_nodes[p_ui->num_layout_nodes - 1];
	p_node->desired_size[0] = (uint16_t)text_width;
	p_node->desired_size[1] = (uint16_t)text_height;

	p_ui->draw_commands[p_ui->num_draw_commands] = (struct cx_ed_ui_draw_command) {
		.p_layout = p_node,
		.b_is_quad = CX_FALSE,
		.details.as_text = *p_text
	};

	p_ui->num_draw_commands++;
}

void cx_ed_ui_row_begin(struct cx_ed_ui* p_ui, enum cx_ed_ui_alignment alignment) {
	cx_ed_ui_layout_node_begin(p_ui, CX_ED_UI_LAYOUT_TYPE_row, alignment);
}

void cx_ed_ui_row_end(struct cx_ed_ui* p_ui) {
	cx_ed_ui_layout_node_end(p_ui);
}

void cx_ed_ui_column_begin(struct cx_ed_ui* p_ui, enum cx_ed_ui_alignment alignment) {
	cx_ed_ui_layout_node_begin(p_ui, CX_ED_UI_LAYOUT_TYPE_column, alignment);
}

void cx_ed_ui_column_end(struct cx_ed_ui* p_ui) {
	cx_ed_ui_layout_node_end(p_ui);
}

void cx_ed_ui_image(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	struct cx_texture* p_texture,
	uint16_t width,
	uint16_t height,
	const struct cx_color* p_color) {

	static const float default_rgba[] = { CX_ED_UI_COLOR_GREEN, 1 };

	const float* p_rgba = p_color ? p_color->rgba : default_rgba;

	cx_ed_ui_layout_node_new(
		p_ui, CX_ED_UI_LAYOUT_TYPE_row, CX_ED_UI_ALIGNMENT_start, p_ui->current_node, s_id, p_callbacks);

	cx_ed_ui_submit_quad(p_ui, width, height, &(struct cx_ed_ui_draw_command_quad) {
		.p_gfx_texture = p_texture ? &p_texture->gfx_texture_ : &shared_resources.default_texture,
		.color = {
			p_rgba[0],
			p_rgba[1],
			p_rgba[2],
			p_rgba[3]
		}
	});
}

void cx_ed_ui_label(
	struct cx_ed_ui* p_ui, 
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	const char* s_text,
	const struct cx_font_render_data* p_font_render_data,
	const struct cx_color* p_color) {
	
	static const struct cx_color default_color = { CX_ED_UI_COLOR_FG, 1 };

	if (p_color == CX_NULL) {
		p_color = &default_color;
	}

	cx_ed_ui_layout_node_new(
		p_ui, CX_ED_UI_LAYOUT_TYPE_row, CX_ED_UI_ALIGNMENT_start, p_ui->current_node, s_id, p_callbacks);

	cx_ed_ui_submit_text(p_ui, &(struct cx_ed_ui_draw_command_text) {
		.p_font_render_data = p_font_render_data ? p_font_render_data : &shared_resources.default_font_render_data,
		.s_text = s_text,
		.color = { CX_COLOR_R(*p_color), CX_COLOR_G(*p_color), CX_COLOR_B(*p_color), CX_COLOR_A(*p_color) }
	});
}

void cx_ed_ui_button(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	const char* s_text) {

	CX_ASSERT(p_callbacks->f_click_cb, UI);

	cx_ed_ui_layout_node_new(
		p_ui, CX_ED_UI_LAYOUT_TYPE_stack, CX_ED_UI_ALIGNMENT_start, p_ui->current_node, CX_NULL, CX_NULL);

	p_ui->current_node = p_ui->num_layout_nodes - 1;

	float text_width;
	float text_height;
	cx_text_mesher_measure(
		s_text, strlen(s_text), &shared_resources.default_font_render_data, 1.0f, &text_width, &text_height);

	// todo: BAD BAD NOT GOOD
	const uint16_t padding_x = 6;
	const uint16_t padding_y = 3;

	cx_ed_ui_image(
		p_ui,
		s_id,
		p_callbacks,
		CX_NULL,
		(uint16_t)text_width + padding_x * 2,
		(uint16_t)text_height + padding_y * 2,
		&(struct cx_color){ CX_ED_UI_COLOR_RED, 1 });

	cx_ed_ui_label(
		p_ui,
		CX_NULL,
		CX_NULL,
		s_text,
		&shared_resources.default_font_render_data,
		&(struct cx_color){ CX_ED_UI_COLOR_FG, 1 });

	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].position[0] += padding_x;
	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].position[1] += padding_y;

	cx_ed_ui_layout_node_end(p_ui);

	struct cx_ed_ui_interactive_widget_record* p_interactive_widget =
		&p_ui->interactive_widgets[p_ui->num_interactive_widgets];
	*p_interactive_widget = (struct cx_ed_ui_interactive_widget_record) {
		.type = CX_ED_UI_INTERACTIVE_WIDGET_TYPE_button,
		.data.button = {
			.draw_command_bg = p_ui->num_draw_commands - 2,
			.draw_command_fg = p_ui->num_draw_commands - 1,
		}
	};

	const uint16_t hitbox_interaction = p_ui->hitboxes[p_ui->num_hitboxes - 1].interaction;
	struct cx_ed_ui_interaction_state* p_hitbox_interaction = &p_ui->interactions[hitbox_interaction];
	p_hitbox_interaction->interactive_widget = p_ui->num_interactive_widgets;

	p_ui->num_interactive_widgets++;
}

void cx_ed_ui_textbox(
	struct cx_ed_ui* p_ui,
	const char* s_id,
	const struct cx_ed_ui_interaction_callbacks* p_callbacks,
	char* p_text_buf,
	size_t text_buf_len) {

	cx_ed_ui_layout_node_new(
		p_ui, CX_ED_UI_LAYOUT_TYPE_stack, CX_ED_UI_ALIGNMENT_start, p_ui->current_node, CX_NULL, CX_NULL);

	p_ui->current_node = p_ui->num_layout_nodes - 1;

	float text_width;
	float text_height;
	cx_text_mesher_measure(
		p_text_buf, strlen(p_text_buf), &shared_resources.default_font_render_data, 1.0f, &text_width, &text_height);

	// todo: BAD BAD NOT GOOD
	const uint16_t content_width = 150;
	const uint16_t border_size = 1;
	const uint16_t padding_x = 6;
	const uint16_t padding_y = 3;

	cx_ed_ui_image(
		p_ui,
		s_id,
		p_callbacks,
		CX_NULL,
		(uint16_t)content_width + (border_size + padding_x) * 2,
		(uint16_t)text_height + (border_size + padding_y) * 2,
		&(struct cx_color){ CX_ED_UI_COLOR_RED, 1 });

	cx_ed_ui_layout_node_new(
		p_ui, CX_ED_UI_LAYOUT_TYPE_stack, CX_ED_UI_ALIGNMENT_start, p_ui->current_node, CX_NULL, CX_NULL);

	p_ui->current_node = p_ui->num_layout_nodes - 1;

	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].b_clip = CX_TRUE;
	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].position[0] += border_size;
	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].position[1] += border_size;
	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].max_size[0] = content_width + padding_x * 2;

	cx_ed_ui_image(
		p_ui,
		s_id,
		p_callbacks,
		CX_NULL,
		(uint16_t)content_width + padding_x * 2,
		(uint16_t)text_height + padding_y * 2,
		&(struct cx_color){ CX_ED_UI_COLOR_RED_DARK, 1 });

	cx_ed_ui_label(
		p_ui,
		CX_NULL,
		CX_NULL,
		p_text_buf,
		&shared_resources.default_font_render_data,
		&(struct cx_color){ CX_ED_UI_COLOR_FG, 1 });

	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].position[0] += padding_x;
	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].position[1] += padding_y;
	p_ui->layout_nodes[p_ui->num_layout_nodes - 1].max_size[0] = content_width + padding_x * 2;

	cx_ed_ui_layout_node_end(p_ui);

	cx_ed_ui_layout_node_end(p_ui);

	struct cx_ed_ui_interactive_widget_record* p_interactive_widget =
		&p_ui->interactive_widgets[p_ui->num_interactive_widgets];
	*p_interactive_widget = (struct cx_ed_ui_interactive_widget_record) {
		.type = CX_ED_UI_INTERACTIVE_WIDGET_TYPE_textbox,
		.data.textbox = {
			.draw_command_border = p_ui->num_draw_commands - 3,
			.draw_command_bg = p_ui->num_draw_commands - 2,
			.draw_command_fg = p_ui->num_draw_commands - 1,
			.p_text_buf = p_text_buf,
			.text_buf_len = text_buf_len
		}
	};

	const uint16_t hitbox_interaction = p_ui->hitboxes[p_ui->num_hitboxes - 1].interaction;
	struct cx_ed_ui_interaction_state* p_hitbox_interaction = &p_ui->interactions[hitbox_interaction];
	p_hitbox_interaction->interactive_widget = p_ui->num_interactive_widgets;

	p_ui->num_interactive_widgets++;
}

static int cx_ed_ui_layout_node_cmp(const void* p_a, const void* p_b) {
	const struct cx_ed_ui_layout_node* p_lhs = p_a;
	const struct cx_ed_ui_layout_node* p_rhs = p_b;

	if (p_lhs->layer < p_rhs->layer) return -1;
	if (p_lhs->layer > p_rhs->layer) return  1;

	if (p_lhs->order < p_rhs->order) return -1;
	if (p_lhs->order > p_rhs->order) return  1;

	return 0;
}

void cx_ed_ui_process_interaction_hovered(struct cx_ed_ui* p_ui, uint16_t interaction) {
	if (interaction == CX_ED_UI_INTERACTION_NONE) {
		return;
	}

	const struct cx_ed_ui_interaction_state* p_interaction = &p_ui->interactions[interaction];

	if (p_interaction->interactive_widget == CX_ED_UI_INTERACTIVE_WIDGET_NONE) {
		return;
	}

	const struct cx_ed_ui_interactive_widget_record* p_interactive_widget =
		&p_ui->interactive_widgets[p_interaction->interactive_widget];

	switch (p_interactive_widget->type) {
		case CX_ED_UI_INTERACTIVE_WIDGET_TYPE_button: {

			struct cx_ed_ui_draw_command* p_draw_command;

			p_draw_command = &p_ui->draw_commands[p_interactive_widget->data.button.draw_command_bg];

			cx_color_f32_cpy(
				&(struct cx_color){ CX_ED_UI_COLOR_RED_BRIGHT, 1 },
				(struct cx_color*)p_draw_command->details.as_quad.color);

			p_draw_command = &p_ui->draw_commands[p_interactive_widget->data.button.draw_command_fg];

			cx_color_f32_cpy(
				&(struct cx_color){ CX_ED_UI_COLOR_FG, 1 },
				(struct cx_color*)p_draw_command->details.as_text.color);

			break;	
		}

		case CX_ED_UI_INTERACTIVE_WIDGET_TYPE_textbox: {
			break;
		}
	}
}

void cx_ed_ui_process_interaction_pressed(struct cx_ed_ui* p_ui, uint16_t interaction) {
	if (interaction == CX_ED_UI_INTERACTION_NONE) {
		return;
	}

	const struct cx_ed_ui_interaction_state* p_interaction = &p_ui->interactions[interaction];

	if (p_interaction->interactive_widget == CX_ED_UI_INTERACTIVE_WIDGET_NONE) {
		return;
	}

	const struct cx_ed_ui_interactive_widget_record* p_interactive_widget =
		&p_ui->interactive_widgets[p_interaction->interactive_widget];

	switch (p_interactive_widget->type) {
		case CX_ED_UI_INTERACTIVE_WIDGET_TYPE_button: {

			struct cx_ed_ui_draw_command* p_draw_command;

			p_draw_command = &p_ui->draw_commands[p_interactive_widget->data.button.draw_command_bg];
			cx_color_f32_cpy(
				&(struct cx_color){ CX_ED_UI_COLOR_RED_DARK, 1 },
				(struct cx_color*)p_draw_command->details.as_quad.color);

			p_draw_command = &p_ui->draw_commands[p_interactive_widget->data.button.draw_command_fg];

			cx_color_f32_cpy(
				&(struct cx_color){ CX_ED_UI_COLOR_FG, 1 },
				(struct cx_color*)p_draw_command->details.as_text.color);

			break;	
		}

		case CX_ED_UI_INTERACTIVE_WIDGET_TYPE_textbox: {
			break;
		}
	}
}

void cx_ed_ui_process_interaction_focused(struct cx_ed_ui* p_ui, uint16_t interaction) {
	if (interaction == CX_ED_UI_INTERACTION_NONE) {
		return;
	}

	const struct cx_ed_ui_interaction_state* p_interaction = &p_ui->interactions[interaction];

	if (p_interaction->interactive_widget == CX_ED_UI_INTERACTIVE_WIDGET_NONE) {
		return;
	}

	const struct cx_ed_ui_interactive_widget_record* p_interactive_widget =
		&p_ui->interactive_widgets[p_interaction->interactive_widget];

	switch (p_interactive_widget->type) {
		case CX_ED_UI_INTERACTIVE_WIDGET_TYPE_button: {
			break;	
		}

		case CX_ED_UI_INTERACTIVE_WIDGET_TYPE_textbox: {
			if (p_ui->text_edit.p_buf == p_interactive_widget->data.textbox.p_text_buf) {
				break;
			}

			cx_text_edit_set_buf(
				&p_ui->text_edit,
				p_interactive_widget->data.textbox.p_text_buf,
				p_interactive_widget->data.textbox.text_buf_len);

			break;
		}
	}
}

void cx_ed_ui_end_frame(struct cx_ed_ui* p_ui, const struct cx_platform_window* p_window) {
	qsort(p_ui->hitboxes, p_ui->num_hitboxes, sizeof(*p_ui->hitboxes), cx_ed_ui_layout_node_cmp);
	qsort(p_ui->draw_commands, p_ui->num_draw_commands, sizeof(*p_ui->draw_commands), cx_ed_ui_layout_node_cmp);

	unsigned int window_width, window_height;
	cx_platform_window_size(p_window, &window_width, &window_height);

	int mouse_x, mouse_y;
	cx_input_mouse_position(&mouse_x, &mouse_y);

	float sx, sy;
	cx_platform_window_normalize_client_coords(p_window, mouse_x, mouse_y, &sx, &sy);

	mouse_x = (int)((float)window_width * sx);
	mouse_y = (int)((float)window_height * sy);

	uint16_t interaction_hovered_new = CX_ED_UI_INTERACTION_NONE;

	for (int32_t i = p_ui->num_hitboxes - 1; i >= 0; --i) {
		const struct cx_ed_ui_hit_box* p_hitbox = &p_ui->hitboxes[i];

		const int16_t left = p_hitbox->p_layout->position[0];
		const int16_t right = (int16_t)(left + p_hitbox->p_layout->size[0]);

		if (mouse_x < left || mouse_x > right) {
			continue;
		}

		const int16_t top = p_hitbox->p_layout->position[1];
		const int16_t bottom = (int16_t)(top + p_hitbox->p_layout->size[1]);

		if (mouse_y < top || mouse_y > bottom) {
			continue;
		}

		interaction_hovered_new = p_hitbox->interaction;

		break;
	}

	struct cx_ed_ui_interaction_state* p_interaction_hovered_new =
		interaction_hovered_new == CX_ED_UI_INTERACTION_NONE ?
			CX_NULL :
			&p_ui->interactions[interaction_hovered_new];

	struct cx_ed_ui_interaction_state* p_interaction_hovered_old =
		p_ui->interaction_hovered == CX_ED_UI_INTERACTION_NONE ?
			CX_NULL :
			&p_ui->interactions[p_ui->interaction_hovered];

	// mouse exit
	if (p_interaction_hovered_new != p_interaction_hovered_old &&
		p_interaction_hovered_old != CX_NULL &&
		p_interaction_hovered_old->callbacks.f_mouse_exit_cb != CX_NULL) {
		
		p_interaction_hovered_old->callbacks.f_mouse_exit_cb(p_interaction_hovered_old->callbacks.p_user_ptr);
	}

	// mouse enter
	if (p_interaction_hovered_new != p_interaction_hovered_old &&
		p_interaction_hovered_new != CX_NULL &&
		p_interaction_hovered_new->callbacks.f_mouse_enter_cb != CX_NULL) {
	
		p_interaction_hovered_new->callbacks.f_mouse_enter_cb(p_interaction_hovered_new->callbacks.p_user_ptr);
	}

	int mouse_dx, mouse_dy;
	cx_input_mouse_delta(&mouse_dx, &mouse_dy);

	int canvas_mouse_dx = (int)((((float)mouse_dx / (float)window_width)) * (float)p_ui->canvas_width);
	int canvas_mouse_dy = (int)((((float)mouse_dy / (float)window_height)) * (float)p_ui->canvas_height);

	// mouse mouse
	if ((canvas_mouse_dx != 0 || canvas_mouse_dy != 0) &&
		p_interaction_hovered_new != CX_NULL &&
		p_interaction_hovered_new->callbacks.f_mouse_move_cb != CX_NULL) {

		p_interaction_hovered_new->callbacks.f_mouse_move_cb(
			canvas_mouse_dx, canvas_mouse_dy, p_interaction_hovered_new->callbacks.p_user_ptr);
	}

	p_ui->interaction_hovered = interaction_hovered_new;

	for (uint16_t i = 0; i < p_ui->num_events; ++i) {
		const struct cx_ed_ui_input_event* p_e = &p_ui->events[i];

		switch (p_e->type) {
			case CX_ED_UI_INPUT_EVENT_TYPE_mouse_button: {
				if (p_interaction_hovered_new != CX_NULL &&
					p_interaction_hovered_new->callbacks.f_mouse_button_cb != CX_NULL) {

					p_interaction_hovered_new->callbacks.f_mouse_button_cb(
						(enum cx_button)i,
						p_e->data.mouse_button.b_is_down,
						p_interaction_hovered_new->callbacks.p_user_ptr);
				}

				if (p_e->data.mouse_button.b_is_down) {
					p_ui->interaction_pressed[p_e->data.mouse_button.button] = interaction_hovered_new;

					// focus_exit
					if (p_ui->interaction_focused != CX_ED_UI_INTERACTION_NONE &&
						interaction_hovered_new != p_ui->interaction_focused) {

						struct cx_ed_ui_interaction_state* p_interaction =
							&p_ui->interactions[p_ui->interaction_focused];

						if (p_interaction->callbacks.f_focus_exit_cb) {
							p_interaction->callbacks.f_focus_exit_cb(
								p_interaction->callbacks.p_user_ptr);
						}

						p_ui->interaction_focused = CX_ED_UI_INTERACTION_NONE;
					}
				} else {
					if (interaction_hovered_new != CX_ED_UI_INTERACTION_NONE &&
						interaction_hovered_new == p_ui->interaction_pressed[i]) {
					
						// click
						if (p_interaction_hovered_new->callbacks.f_click_cb) {
							p_interaction_hovered_new->callbacks.f_click_cb(
								p_interaction_hovered_new->callbacks.p_user_ptr);
						}

						if (p_ui->interaction_focused != interaction_hovered_new) {
							// focus_enter
							if (p_interaction_hovered_new->callbacks.f_focus_enter_cb) {
								p_interaction_hovered_new->callbacks.f_focus_enter_cb(
									p_interaction_hovered_new->callbacks.p_user_ptr);
							}

							p_ui->interaction_focused = interaction_hovered_new;
						}
					}

					p_ui->interaction_pressed[i] = CX_ED_UI_INTERACTION_NONE;
				}

				break;
			}

			case CX_ED_UI_INPUT_EVENT_TYPE_keys: {

				break;
			}

			case CX_ED_UI_INPUT_EVENT_TYPE_charcode: {

				if (p_ui->interaction_focused == CX_ED_UI_INTERACTION_NONE ||
					p_ui->interactions[p_ui->interaction_focused].interactive_widget
						== CX_ED_UI_INTERACTIVE_WIDGET_NONE ||
					p_ui->interactive_widgets[p_ui->interactions[p_ui->interaction_focused].interactive_widget].type
						!= CX_ED_UI_INTERACTIVE_WIDGET_TYPE_textbox) {
					
					break;
				}
				
				char c = (char)p_e->data.charcode.code;

				if (iscntrl(c)) {
					break;
				}

				cx_text_edit_insert(&p_ui->text_edit, &c, 1);

				break;	
			}
		}
	}

	// cleanup

	p_ui->num_hitboxes = 0;

	cx_ed_ui_persistent_state_pool_remove_inactive(&p_ui->window_pool);
	cx_ed_ui_persistent_state_pool_remove_inactive(&p_ui->interaction_pool);

	for (uint16_t i = 0; i < CX_BUTTON_MAX_; ++i) {
		if (p_ui->interaction_pressed[i] != CX_ED_UI_INTERACTION_NONE &&
			!p_ui->interactions[p_ui->interaction_pressed[i]].pool_slot.b_is_occupied) {
			p_ui->interaction_pressed[i] = CX_ED_UI_INTERACTION_NONE;
		}
	}

	cx_ed_ui_process_interaction_hovered(p_ui, p_ui->interaction_hovered);
	cx_ed_ui_process_interaction_pressed(p_ui, p_ui->interaction_pressed[CX_BUTTON_mouse_left]);
	cx_ed_ui_process_interaction_focused(p_ui, p_ui->interaction_focused);
}

void cx_ed_ui_init_shared_resources(void) {
	static int b_done = CX_FALSE;
	if (b_done) {
		return;
	}

	char* p_vsource;
	char* p_fsource;
	struct cx_gfx_program_source source;

	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/ui_quad.vert", (void**)&p_vsource, 0) == CX_ERROR_none, UI);
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/ui_quad.frag", (void**)&p_fsource, 0) == CX_ERROR_none, UI);

	source = (struct cx_gfx_program_source){
		.s_vertex_stage_source = p_vsource,
		.s_fragment_stage_source = p_fsource
	};

	CX_ASSERT(cx_gfx_program_create(&shared_resources.program) == CX_ERROR_none, UI);
	CX_ASSERT(cx_gfx_program_build(&shared_resources.program, &source) == CX_ERROR_none, UI);

	cx_io_file_free(p_vsource);
	cx_io_file_free(p_fsource);

	CX_ASSERT(cx_gfx_program_refl_param_block(
		&shared_resources.program, "blk_camera", &shared_resources.program_pblk_camera), UI);
	CX_ASSERT(cx_gfx_program_param_buffer_create(
		&shared_resources.program_pbuf_camera, shared_resources.program_pblk_camera.size_) == CX_ERROR_none, UI);
	CX_ASSERT(cx_gfx_program_refl_opaque_param(
		&shared_resources.program,
		"u_texture",
		&shared_resources.program_sampler2d), UI);

	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/ui_text.vert", (void**)&p_vsource, 0) == CX_ERROR_none, UI);
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/ui_text.frag", (void**)&p_fsource, 0) == CX_ERROR_none, UI);

	source = (struct cx_gfx_program_source){
		.s_vertex_stage_source = p_vsource,
		.s_fragment_stage_source = p_fsource
	};

	CX_ASSERT(cx_gfx_program_create(&shared_resources.text_program) == CX_ERROR_none, UI);
	CX_ASSERT(cx_gfx_program_build(&shared_resources.text_program, &source) == CX_ERROR_none, UI);

	cx_io_file_free(p_vsource);
	cx_io_file_free(p_fsource);

	CX_ASSERT(cx_gfx_program_refl_param_block(
		&shared_resources.text_program, "blk_camera", &shared_resources.text_program_pblk_camera), UI);
	CX_ASSERT(cx_gfx_program_param_buffer_create(
		&shared_resources.text_program_pbuf_camera, shared_resources.text_program_pblk_camera.size_) == CX_ERROR_none,
		UI);
	CX_ASSERT(cx_gfx_program_refl_param_block(
		&shared_resources.text_program, "blk_object", &shared_resources.text_program_pblk_object), UI);
	CX_ASSERT(cx_gfx_program_param_buffer_create(
		&shared_resources.text_program_pbuf_object, shared_resources.text_program_pblk_object.size_) == CX_ERROR_none,
		UI);
	CX_ASSERT(cx_gfx_program_refl_opaque_param(
		&shared_resources.text_program,
		"u_texture",
		&shared_resources.text_program_sampler2d), UI);

	uint8_t white_pixel[] = { 0xFF, 0xFF, 0xFF };
	struct cx_image white_image = {
		.width = 1,
		.height = 1,
		.pixel_data_format = {
			.pixel_format = CX_PIXEL_FORMAT_rgb,
			.pixel_type = CX_PIXEL_TYPE_u8
		},
		.p_pixel_data = white_pixel
	};

	cx_gfx_texture_create(
		&shared_resources.default_texture, white_image.width, white_image.height, CX_PIXEL_FORMAT_rgba);
	cx_gfx_texture_set_data(
		&shared_resources.default_texture, white_image.p_pixel_data, &white_image.pixel_data_format);

	CX_ASSERT(cx_asset_cache_find_by_name(CX_ASSET_TYPE_FONT, "default_8x14", &shared_resources.default_font_ref), UI);
	struct cx_font* p_font = cx_asset_cache_acquire(&shared_resources.default_font_ref);

	shared_resources.default_font_glyph_atlas_layout = (struct cx_texture_atlas_layout) {
		.p_entries = shared_resources.default_font_glyph_atlas_layout_entries
	};

	struct cx_image font_atlas_image;
	cx_font_create_atlas(p_font, &font_atlas_image, &shared_resources.default_font_glyph_atlas_layout);

	cx_font_free_glyph_bitmap_buffer(p_font);

	cx_gfx_texture_create(
		&shared_resources.default_font_glyph_atlas_texture,
		font_atlas_image.width, font_atlas_image.height,
		CX_PIXEL_FORMAT_red);

	cx_gfx_texture_set_data(
		&shared_resources.default_font_glyph_atlas_texture,
		font_atlas_image.p_pixel_data,
		&font_atlas_image.pixel_data_format);

	free(font_atlas_image.p_pixel_data);

	shared_resources.default_font_render_data = (struct cx_font_render_data) {
		.p_font = p_font,
		.p_glyph_atlas_layout = &shared_resources.default_font_glyph_atlas_layout,
		.p_glyph_texture = &shared_resources.default_font_glyph_atlas_texture
	};

	b_done = CX_TRUE;
}

void cx_ed_ui_draw(struct cx_ed_ui* p_ui) {
	struct cx_mesh_vertex_attribute mesh_attributes[] = {
		(struct cx_mesh_vertex_attribute) {
			.index = 0,
			.vertex_buffer_index = 0,
			.format = {
				.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
				.count = 3
			},
			.layout = {
				.offset = 0,
				.stride = sizeof(float) * 9
			}
		},
		(struct cx_mesh_vertex_attribute) {
			.index = 1,
			.vertex_buffer_index = 0,
			.format = {
				.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
				.count = 4
			},
			.layout = {
				.offset = sizeof(float) * 3,
				.stride = sizeof(float) * 9
			}
		},
		(struct cx_mesh_vertex_attribute) {
			.index = 2,
			.vertex_buffer_index = 0,
			.format = {
				.type = CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
				.count = 2
			},
			.layout = {
				.offset = sizeof(float) * 7,
				.stride = sizeof(float) * 9
			}
		}
	};

	float vertices[6 * 9];

	struct cx_mesh_vertex_buffer vertex_buffer = {
		.p_bytes = vertices,
		.size = sizeof(vertices)
	};

	struct cx_mesh_data mesh_data = {
		.layout = {
			.num_vertex_buffers = 1,
			.p_attributes = mesh_attributes,
			.num_attributes = 3,
			.draw_mode = CX_MESH_DRAW_MODE_triangles
		},
		.p_vertex_buffers = &vertex_buffer,
		.vertex_count = 6,
	};


	for (int32_t i = 0; i < p_ui->num_draw_commands; ++i) {
		struct cx_ed_ui_draw_command* p_draw_command = &p_ui->draw_commands[i];

		if (p_draw_command->p_layout->b_clip) {
			if (!p_ui->b_clip) {
				p_ui->b_clip = CX_TRUE;
				glEnable(GL_SCISSOR_TEST);
			}

			if (p_ui->clip_rect_pos[0] != p_draw_command->p_layout->clip_pos[0] ||
				p_ui->clip_rect_pos[1] != p_draw_command->p_layout->clip_pos[1] ||
				p_ui->clip_rect_size[0] != p_draw_command->p_layout->clip_size[0] ||
				p_ui->clip_rect_size[1] != p_draw_command->p_layout->clip_size[1]) {

				p_ui->clip_rect_pos[0] = p_draw_command->p_layout->clip_pos[0];
				p_ui->clip_rect_pos[1] = p_draw_command->p_layout->clip_pos[1];
				p_ui->clip_rect_size[0] = p_draw_command->p_layout->clip_size[0];
				p_ui->clip_rect_size[1] = p_draw_command->p_layout->clip_size[1];

				glScissor(
					p_ui->clip_rect_pos[0],
					(int16_t)p_ui->canvas_height - p_ui->clip_rect_pos[1] - (int16_t)p_ui->clip_rect_size[1],
					p_ui->clip_rect_size[0],
					p_ui->clip_rect_size[1]);
			}
		} else if (p_ui->b_clip) {
			p_ui->b_clip = CX_FALSE;
			glDisable(GL_SCISSOR_TEST);
		}

		if (!p_ui->draw_commands[i].b_is_quad) {
			struct cx_text_mesher_input text_mesher_input = {
				.s_text = p_draw_command->details.as_text.s_text,
				.style = {
					.p_font_render_data = p_draw_command->details.as_text.p_font_render_data,
					.scale = 1
				}
			};
			cx_color_f32_cpy((void*)p_draw_command->details.as_text.color, &text_mesher_input.style.color);
			
			struct cx_text_mesher_output text_mesher_output;
			size_t num_text_meshes;

			cx_text_mesher_generate(&text_mesher_input, 1, &text_mesher_output, &num_text_meshes);

			struct cx_gfx_mesh text_mesh;
			cx_gfx_mesh_create(&text_mesher_output.mesh_data, CX_GFX_BUFFER_USAGE_static, &text_mesh);

			cx_text_mesher_free(&text_mesher_output, 1);

			float camera[16];
			matrix_make_orthographic_projection(0, (float)p_ui->canvas_width, 0, (float)p_ui->canvas_height, -1, 1, camera);

			float vertex_matrix[16];
			matrix_make_translation(
				p_draw_command->p_layout->position[0],
				p_draw_command->p_layout->position[1] +
					(float)p_draw_command->details.as_text.p_font_render_data->p_font->line_height_ -
					(float)p_draw_command->details.as_text.p_font_render_data->p_font->descent_,
				0,
				vertex_matrix);
			vertex_matrix[5] *= -1;

			cx_gfx_program_bind(&shared_resources.text_program);

			cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
				.p_block = &shared_resources.text_program_pblk_camera,
				.p_buffer = &shared_resources.text_program_pbuf_camera
			}));

			cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
				.p_block = &shared_resources.text_program_pblk_object,
				.p_buffer = &shared_resources.text_program_pbuf_object
			}));

			cx_gfx_program_param_buffer_set(&shared_resources.text_program_pbuf_camera, 0, 0, camera);
			cx_gfx_program_param_buffer_set(&shared_resources.text_program_pbuf_object, 0, 0, vertex_matrix);

			cx_gfx_program_opaque_param_bind_resource(&((struct cx_gfx_program_opaque_param_binding) {
				.p_param = &shared_resources.text_program_sampler2d,
				.p_resource = p_draw_command->details.as_text.p_font_render_data->p_glyph_texture
			}));

			cx_gfx_mesh_draw(&text_mesh);
			cx_gfx_mesh_destroy(&text_mesh);

			continue;
		}

		float camera[16];
		matrix_make_orthographic_projection(0, (float)p_ui->canvas_width, 0, (float)p_ui->canvas_height, -1, 1, camera);

		cx_gfx_program_bind(&shared_resources.program);

		cx_gfx_program_param_block_bind_buffer(&((struct cx_gfx_program_param_block_binding) {
			.p_block = &shared_resources.program_pblk_camera,
			.p_buffer = &shared_resources.program_pbuf_camera
		}));

		cx_gfx_program_param_buffer_set(&shared_resources.program_pbuf_camera, 0, 0, camera);

		//CX_LOG_FMT(INFO, UI, "draw(): layout=%p, quad.pos={ %d, %d }, quad.size={ %d, %d }\n",
		//	p_draw_command->p_layout,
		//	p_draw_command->p_layout->position[0],
		//	p_draw_command->p_layout->position[1],
		//	p_draw_command->p_layout->size[0],
		//	p_draw_command->p_layout->size[1]);

		const float left   = p_draw_command->p_layout->position[0];
		const float right  = p_draw_command->p_layout->size[0] + left;
		const float top    = p_draw_command->p_layout->position[1];
		const float bottom = p_draw_command->p_layout->size[1] + top;

		const float r = p_draw_command->details.as_quad.color[0];
		const float g = p_draw_command->details.as_quad.color[1];
		const float b = p_draw_command->details.as_quad.color[2];
		const float a = p_draw_command->details.as_quad.color[3];

		const float v0[] = { left,  top,    0,   r, g, b, a,   0, 1 };
		const float v1[] = { left,  bottom, 0,   r, g, b, a,   0, 0 };
		const float v2[] = { right, top,    0,   r, g, b, a,   1, 1 };
		const float v3[] = { right, bottom, 0,   r, g, b, a,   1, 0 };

		memcpy(&vertices[0 * 9], v0, sizeof(v0));
		memcpy(&vertices[1 * 9], v1, sizeof(v1));
		memcpy(&vertices[2 * 9], v2, sizeof(v2));

		memcpy(&vertices[3 * 9], v3, sizeof(v3));
		memcpy(&vertices[4 * 9], v2, sizeof(v2));
		memcpy(&vertices[5 * 9], v1, sizeof(v1));
		
		cx_gfx_program_opaque_param_bind_resource(&((struct cx_gfx_program_opaque_param_binding) {
			.p_param = &shared_resources.program_sampler2d,
			.p_resource = p_draw_command->details.as_quad.p_gfx_texture
		}));

		struct cx_gfx_mesh gfx_mesh;
		cx_gfx_mesh_create(&mesh_data, CX_GFX_BUFFER_USAGE_static, &gfx_mesh);
		cx_gfx_mesh_draw(&gfx_mesh);
		cx_gfx_mesh_destroy(&gfx_mesh);
	}

	p_ui->b_clip = CX_FALSE;
	p_ui->clip_rect_pos[0] = 0;
	p_ui->clip_rect_pos[1] = 0;
	p_ui->clip_rect_size[0] = 0;
	p_ui->clip_rect_size[1] = 0;
	glDisable(GL_SCISSOR_TEST);

	p_ui->current_window = CX_ED_UI_WINDOW_NONE;
	p_ui->current_node = CX_ED_UI_LAYOUT_NODE_NONE;
	p_ui->num_layout_nodes = 0;
	p_ui->num_hitboxes = 0;
	p_ui->num_interactive_widgets = 0;
	p_ui->num_draw_commands = 0;
	p_ui->num_events = 0;
}

int cx_ed_ui_persistent_state_pool_try_get(
	struct cx_ed_ui_persistent_state_pool* p_pool,
	const char* s_id,
	uint16_t* p_out_slot) {

	uint16_t first_free_slot = p_pool->count == 0 ? 0 : p_pool->capacity;

	uint16_t checked = 0;
	for (uint16_t i = 0; i < p_pool->capacity && checked < p_pool->count; ++i) {
		struct cx_ed_ui_persistent_state_pool_slot* p_slot =
			(void*)((uint8_t*)p_pool->p_slots + p_pool->slot_size * i);

		if (!p_slot->b_is_occupied) {
			if (first_free_slot == p_pool->capacity) {
				first_free_slot = i;
			}
			continue;
		}

		if (cx_str_eq(p_slot->id, s_id)) {
			p_slot->inactive_frames = 0;
			*p_out_slot = i;
			return CX_TRUE;
		}

		checked++;
	}

	CX_ASSERT(p_pool->count < p_pool->capacity - 1, UI);

	if (first_free_slot == p_pool->capacity) {
		first_free_slot = p_pool->count;
	}

	struct cx_ed_ui_persistent_state_pool_slot* p_slot =
		(void*)((uint8_t*)p_pool->p_slots + p_pool->slot_size * first_free_slot);
	*p_slot = (struct cx_ed_ui_persistent_state_pool_slot) {
		.b_is_occupied = CX_TRUE
	};
	strcpy(p_slot->id, s_id);

	p_pool->count++;

	*p_out_slot = first_free_slot;

	return CX_FALSE;
}

void cx_ed_ui_persistent_state_pool_remove_inactive(
	struct cx_ed_ui_persistent_state_pool* p_pool) {

	uint16_t checked = 0;
	for (uint16_t i = 0; i < p_pool->capacity && checked < p_pool->count; ++i) {
		struct cx_ed_ui_persistent_state_pool_slot* p_slot =
			(void*)((uint8_t*)p_pool->p_slots + p_pool->slot_size * i);

		if (!p_slot->b_is_occupied) {
			continue;
		}

		if (p_slot->inactive_frames > CX_ED_UI_MAX_INACTIVE_FRAMES) {
			p_slot->b_is_occupied = CX_FALSE;
			p_pool->count--;
		}

		checked++;
	}
}
