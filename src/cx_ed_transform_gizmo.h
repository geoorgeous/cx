#ifndef CX_ED_TRANSFORM_GIZMO_H
#define CX_ED_TRANSFORM_GIZMO_H

#include <stddef.h>
#include <stdint.h>

#include "cx_gfx_shader_program_interface.h"
#include "transform.h"

#define CX_LOG_CAT_GIZMO "gizmo"

enum cx_transform_gizmo_mode {
	CX_TRANSFORM_GIZMO_MODE_translate,
	CX_TRANSFORM_GIZMO_MODE_rotate,
	CX_TRANSFORM_GIZMO_MODE_scale
};

enum cx_transform_gizmo_interaction_state {
	CX_TRANSFORM_GIZMO_INTERACTION_STATE_idle,
	CX_TRANSFORM_GIZMO_INTERACTION_STATE_in_progress,
	CX_TRANSFORM_GIZMO_INTERACTION_STATE_ended,
	CX_TRANSFORM_GIZMO_INTERACTION_STATE_cancelled
};

struct cx_transform_gizmo_control_render_data {
	struct cx_gfx_shader_program_input_block material_shader_program_input_block;
	struct cx_gfx_shader_program_input_block object_shader_program_input_block;
	struct {
		float color[4];
	} material_block_data;
	struct {
		float transform[16];
		uint32_t object_id;
	} object_block_data;
};

struct cx_transform_gizmo {
	enum cx_transform_gizmo_mode mode;
	enum cx_transform_gizmo_interaction_state interaction_state;
	int      b_use_local_axes;
	float    gizmo_transform[16];
	uint32_t active_control_id;

	struct {
		struct transform initial_target_transform;
		float manipulation_origin[3];
	} drag_state;

	struct {
		struct cx_transform_gizmo_control_render_data t[7];
		struct cx_transform_gizmo_control_render_data r[4];
		struct cx_transform_gizmo_control_render_data s[7];
	} render_data;
};

struct cx_transform_gizmo_interaction_result {
	enum cx_transform_gizmo_interaction_state state;
	struct transform transform;
};

struct cx_asset_package;

void cx_transform_gizmo_init_shared_resources(void);

void cx_transform_gizmo_init_controls(struct cx_transform_gizmo* p_gizmo);

enum cx_transform_gizmo_interaction_state cx_transform_gizmo_update(
	struct cx_transform_gizmo* p_gizmo,
	const struct transform* p_target_transform,
	uint32_t picked_id,
	const float* p_view_pos,
	float gizmo_view_scale,
	const float* p_cursor_world_ray,
	struct transform* p_out_transform);

struct cx_render_pipeline;
struct cx_render_draw_command_buffer;

void cx_transform_gizmo_record_draw_commands(
	const struct cx_transform_gizmo* p_gizmo,
	const struct cx_render_pipeline* p_render_pipeline,
	struct cx_render_draw_command_buffer* p_render_draw_command_buffer);

#endif
