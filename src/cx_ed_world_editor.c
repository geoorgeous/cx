#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_asset_defs.h"
#include "cx_blueprint.h"
#include "cx_cmp_collider.h"
#include "cx_cmp_rigidbody.h"
#include "cx_cmp_static_mesh.h"
#include "cx_command.h"
#include "cx_console.h"
#include "cx_dbg.h"
#include "cx_ed_action.h"
#include "cx_ed_asset_library.h"
#include "cx_ed_transform_gizmo.h"
#include "cx_ed_world_editor.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_render_pass.h"
#include "cx_macro.h"
#include "cx_mesh_data.h"
#include "cx_object_id_capturer.h"
#include "cx_result.h"
#include "cx_shader.h"
#include "cx_str.h"
#include "cx_var.h"
#include "cx_world.h"
#include "cx_world_blueprint.h"
#include "cx_world_renderer.h"
#include "input.h"
#include "matrix.h"
#include "physics.h"
#include "platform_window.h"
#include "vector.h"

#include "cx_render_draw_command.h"
#include "cx_render_pipeline.h"

#define CX_ACTION_DEF(NAME, ...) \
	struct cx_ed_action_##NAME##_ctx { \
		__VA_ARGS__ \
	}; \
	static void cx_ed_action_##NAME##_do(void* p_ctx); \
	static void cx_ed_action_##NAME##_undo(void* p_ctx); \
	static const struct cx_ed_action_def action_##NAME##_def = { \
		.context_size = sizeof(struct cx_ed_action_##NAME##_ctx), \
		.context_alignment = CX_ALIGNOF(struct cx_ed_action_##NAME##_ctx), \
		.f_do = cx_ed_action_##NAME##_do, \
		.f_undo = cx_ed_action_##NAME##_undo \
	}

#define CX_ACTION_EXECUTE(NAME, P_CTX)\
	cx_ed_action_history_execute( \
		&ed.action_history, \
		&action_##NAME##_def, \
		P_CTX)

static struct {
	struct platform_window* p_window;

	struct cx_asset_ref world_blueprint_asset_ref;

	struct cx_ed_action_history action_history;

	struct cx_asset_ref asset_ref_shader_lit;
	struct cx_asset_ref asset_ref_shader_flat;
	struct cx_asset_ref asset_ref_shader_editor_grid;
	struct cx_asset_ref asset_ref_shader_object_id;

	struct cx_render_draw_command render_draw_commands[CX_WORLD_MAX_ENTITIES];
	struct cx_render_pipeline render_pipeline_lit;
	struct cx_render_pipeline render_pipeline_flat;
	struct cx_render_pipeline render_pipeline_editor_grid;
	struct cx_render_pipeline render_pipeline_object_id;
	struct cx_gfx_mesh dummy_mesh;

	struct cx_transform_gizmo gizmo;

	struct cx_world world;
	struct physics_world physics_world;

	struct cx_object_id_capturer object_id_capturer;

	uint32_t object_id_at_cursor;
	uint16_t selected_entity_id;
	uint16_t entity_id_at_cursor;

	// buffers for command flog builder
	char                   flog_builder_str_buf[1024];
	struct cx_flog_style   flog_builder_style_buf[8];
	struct cx_flog_span    flog_builder_span_buf[8];
	struct cx_flog_builder flog_builder;

	// editor camera
	struct {
		float position[3];
		float pitch;
		float yaw;
		float projection_matrix[16];
		float view_matrix[16];
	} camera;
} ed;

// COMMANDS

cx_result cx_ed_world_editor_load_world_from_world_blueprint(const char* s_asset_name);
int cx_cmd_world_editor_open_world_blueprint(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

cx_result cx_ed_world_editor_save_world_to_world_blueprint(const char* s_name);
int cx_cmd_world_editor_save_world_blueprint(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

int cx_cmd_world_editor_spawn_blueprint(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

cx_result cx_ed_world_editor_entity_new(uint16_t* p_out_entity_id);
int cx_cmd_world_editor_entity_new(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

cx_result cx_ed_world_editor_entity_destroy(uint16_t entity_id);
int cx_cmd_world_editor_entity_destroy(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

cx_result cx_ed_world_editor_entity_clone(uint16_t entity_id);
int cx_cmd_world_editor_entity_clone(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

cx_result cx_ed_world_editor_entity_set_parent(uint16_t entity_id, uint16_t parent_entity_id);
int cx_cmd_world_editor_entity_set_parent(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

cx_result cx_ed_world_editor_entity_add_component(uint16_t entity_id, const struct cx_component_type* p_type);
int cx_cmd_world_editor_entity_add_component(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

cx_result cx_ed_world_editor_entity_remove_component(uint16_t entity_id, const struct cx_component_type* p_type);
int cx_cmd_world_editor_entity_remove_component(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

// SET ENTITY TRANSFORM

CX_ACTION_DEF(set_entity_transform,
	struct cx_world* p_world;
	uint16_t entity_id;
	struct transform transform_old;
	struct transform transform;
);

void cx_ed_action_set_entity_transform_do(void* p_ctx) {
	const struct cx_ed_action_set_entity_transform_ctx* p = p_ctx;
	struct transform* p_t = cx_world_entity_get_transform(p->p_world, p->entity_id);
	transform_set_world_position(p_t, p->transform.world_position);
	transform_set_world_scale(p_t, p->transform.world_scale);
	transform_set_world_rotation(p_t, p->transform.world_rotation);
}

void cx_ed_action_set_entity_transform_undo(void* p_ctx) {
	const struct cx_ed_action_set_entity_transform_ctx* p = p_ctx;
	struct transform* p_t = cx_world_entity_get_transform(p->p_world, p->entity_id);
	transform_set_world_position(p_t, p->transform_old.world_position);
	transform_set_world_scale(p_t, p->transform_old.world_scale);
	transform_set_world_rotation(p_t, p->transform_old.world_rotation);
}

uint16_t cx_ed_destroy_entity(uint16_t entity_id);
uint16_t cx_ed_set_entity_parent(uint16_t entity_id, uint16_t parent_entity_id);

static void cx_ed_world_editor_on_key(const void* p_e, void* p_user_ptr);

void cx_ed_world_editor_init(struct platform_window* p_window, const char* s_world_blueprint_asset_name) {
	ed.p_window = p_window;

	ed.flog_builder = (struct cx_flog_builder) {
		.p_buf = ed.flog_builder_str_buf,
		.p_styles = ed.flog_builder_style_buf,
		.p_spans = ed.flog_builder_span_buf
	};

	ed.entity_id_at_cursor = CX_ENTITY_ID_INVALID;
	ed.selected_entity_id = CX_ENTITY_ID_INVALID;

	CX_NEW_CONSOLE_COMMAND(
		"w.load",
		"Open a new or existing world blueprint to edit", cx_cmd_world_editor_open_world_blueprint, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("world_blueprint", "The asset's name"), REQUIRED));

	CX_NEW_CONSOLE_COMMAND(
		"w.save",
		"Save the current world blueprint to disk", cx_cmd_world_editor_save_world_blueprint, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("filepath", "The location to save the asset"), OPTIONAL));

	CX_NEW_CONSOLE_COMMAND(
		"w.spawnbp",
		"Spawn a blueprint asset in the world, creating the entity hierarchy and components.",
		cx_cmd_world_editor_spawn_blueprint, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("blueprint", "The blueprint asset's ID or name"), REQUIRED));

	CX_NEW_CONSOLE_COMMAND(
		"e.new",
		"Create a new entity", cx_cmd_world_editor_entity_new, CX_NULL,
		CX_CONSOLE_COMMAND_NO_PARAMS);

	CX_NEW_CONSOLE_COMMAND(
		"e.destroy",
		"Destroy an entity", cx_cmd_world_editor_entity_destroy, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("entity", "The entity to destroy"), REQUIRED));

	CX_NEW_CONSOLE_COMMAND(
		"e.clone",
		"Clone an existing entity", cx_cmd_world_editor_entity_clone, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("entity", "The entity to destroy"), REQUIRED));
	
	CX_NEW_CONSOLE_COMMAND(
		"e.parent",
		"Set the parent of an entity", cx_cmd_world_editor_entity_set_parent, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("entity", "The entity to asign the parent to"), REQUIRED),
		CX_CONSOLE_COMMAND_PARAM(STRING("parent", "The parent to asign to the entity"), REQUIRED));

	CX_NEW_CONSOLE_COMMAND(
		"e.cmp.add",
		"Add a component to an entity", cx_cmd_world_editor_entity_add_component, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("entity", "The entity to add the component to"), REQUIRED),
		CX_CONSOLE_COMMAND_PARAM(STRING("component", "The name of the component type to add"), REQUIRED));

	CX_NEW_CONSOLE_COMMAND(
		"e.cmp.remove",
		"Remove a component from an entity", cx_cmd_world_editor_entity_remove_component, CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("entity", "The entity to remove the component from"), REQUIRED),
		CX_CONSOLE_COMMAND_PARAM(STRING("component", "The name of the component type to remove"), REQUIRED));
	
	struct cx_shader* p_shader;

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_lit", &ed.asset_ref_shader_lit);
	p_shader = cx_asset_cache_acquire(&ed.asset_ref_shader_lit);
	cx_shader_load_device_program(p_shader);

	ed.render_pipeline_lit = (struct cx_render_pipeline) {
		.p_shader = p_shader,
		.state = {
			.flags =
				CX_RENDER_PIPELINE_FLAG_depth_test_enabled |
				CX_RENDER_PIPELINE_FLAG_depth_writes_enabled,
			.depth_test_func = CX_DEPTH_TEST_FUNC_less,
			.cull_mode = CX_CULL_MODE_back
		}
	};

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_flat", &ed.asset_ref_shader_flat);
	p_shader = cx_asset_cache_acquire(&ed.asset_ref_shader_flat);
	cx_shader_load_device_program(p_shader);

	ed.render_pipeline_flat = (struct cx_render_pipeline) {
		.p_shader = p_shader,
		.state = {
			.flags =
				CX_RENDER_PIPELINE_FLAG_depth_test_enabled |
				CX_RENDER_PIPELINE_FLAG_depth_writes_enabled,
			.depth_test_func = CX_DEPTH_TEST_FUNC_less,
			.cull_mode = CX_CULL_MODE_back
		}
	};

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_editor_grid", &ed.asset_ref_shader_editor_grid);
	p_shader = cx_asset_cache_acquire(&ed.asset_ref_shader_editor_grid);
	cx_shader_load_device_program(p_shader);

	ed.render_pipeline_editor_grid = (struct cx_render_pipeline) {
		.p_shader = p_shader,
		.state = {
			.flags =
				CX_RENDER_PIPELINE_FLAG_depth_test_enabled |
				CX_RENDER_PIPELINE_FLAG_depth_writes_enabled |
				CX_RENDER_PIPELINE_FLAG_blend_enabled,
			.depth_test_func = CX_DEPTH_TEST_FUNC_less,
			.blend_src_func = CX_BLEND_FUNC_src_alpha,
			.blend_dst_func = CX_BLEND_FUNC_one_minus_src_alpha,
			.cull_mode = CX_CULL_MODE_none
		}
	};

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_object_id", &ed.asset_ref_shader_object_id);
	p_shader = cx_asset_cache_acquire(&ed.asset_ref_shader_object_id);
	cx_shader_load_device_program(p_shader);

	ed.render_pipeline_object_id = (struct cx_render_pipeline) {
		.p_shader = p_shader,
		.state = {
			.flags =
				CX_RENDER_PIPELINE_FLAG_depth_test_enabled |
				CX_RENDER_PIPELINE_FLAG_depth_writes_enabled,
			.depth_test_func = CX_DEPTH_TEST_FUNC_always,
			.cull_mode = CX_CULL_MODE_back
		}
	};

	cx_gfx_mesh_create(
		&(struct cx_mesh_data) {
			.layout.draw_mode = CX_MESH_DRAW_MODE_triangles
		},
		CX_GFX_BUFFER_USAGE_static,
		&ed.dummy_mesh);
	ed.dummy_mesh.num_elements_ = 3;

	cx_transform_gizmo_init_shared_resources();
	cx_transform_gizmo_init_controls(&ed.gizmo);

	struct cx_component_pool_def world_component_pool_defs[] = {
		{ &cmp_type_static_mesh,  CX_WORLD_MAX_ENTITIES },
		{ &cmp_type_collider,     512 },
		{ &cmp_type_rigidbody,    128 }
	};

	cx_world_init(&ed.world, world_component_pool_defs, CX_ARRAY_LEN(world_component_pool_defs));

	physics_world_init(&ed.physics_world);
	physics_world_add_solver(&ed.physics_world, physics_collision_solver_impulse);
	physics_world_add_solver(&ed.physics_world, physics_collision_solver_smooth_positions);

	input_event_subscribe(INPUT_EVENT_key, cx_ed_world_editor_on_key, 0);

	cx_ed_world_editor_load_world_from_world_blueprint(s_world_blueprint_asset_name);

	struct cx_asset_ref blueprint_asset_ref;
	cx_asset_cache_find_by_name(CX_ASSET_TYPE_BLUEPRINT, "test", &blueprint_asset_ref);

	const struct cx_blueprint* p_blueprint = cx_asset_cache_acquire(&blueprint_asset_ref);

	cx_world_instantiate_blueprint(&ed.world, p_blueprint);

	cx_asset_cache_release(&blueprint_asset_ref);
}

void cx_ed_world_editor_shutdown(void) {
	cx_world_free(&ed.world);
}

void cx_ed_world_editor_update(double dt_seconds) {
	float move_direction[3] = {0};
	if (!cx_console_get()->b_is_input_enabled) {
		if (input_frame_is_key_down(KEY_a)) {
			move_direction[0] -= 1;
		}
		if (input_frame_is_key_down(KEY_d)) {
			move_direction[0] += 1;
		}
		if (input_frame_is_key_down(KEY_s)) {
			move_direction[2] += 1;
		}
		if (input_frame_is_key_down(KEY_w)) {
			move_direction[2] -= 1;
		}
		if (input_frame_is_key_down(KEY_space)) {
			move_direction[1] += 1;
		}
		if (input_frame_is_key_down(KEY_c)) {
			move_direction[1] -= 1;
		}

		if (input_frame_is_mouse_button_down(MOUSE_BUTTON_right)) {
			int mouse_delta_x;
			int mouse_delta_y;
			input_frame_mouse_delta(&mouse_delta_x, &mouse_delta_y);

			ed.camera.pitch += (float)mouse_delta_y * 0.01f;
			ed.camera.yaw += (float)mouse_delta_x * 0.01f;
		}
	}
	
	float pitch_rotation_matrix[16];
	matrix_make_rotation_x(ed.camera.pitch, pitch_rotation_matrix);

	float yaw_rotation_matrix[16];
	matrix_make_rotation_y(ed.camera.yaw, yaw_rotation_matrix);
	
	float rotation_matrix[16];
	matrix_multiply(pitch_rotation_matrix, yaw_rotation_matrix, rotation_matrix);

	if (!vec3_is_zero(move_direction)) {
		const float speed = input_frame_is_key_down(KEY_shift_left) ? 50.f : 7.f;
		float offset[4] = { 0, 0, 0, 1 };

		vec3_norm(move_direction, move_direction);
		vec3_mul_s(move_direction, speed * (float)dt_seconds, offset);

		float camera_x_axis[3];
		camera_x_axis[0] = rotation_matrix[0];
		camera_x_axis[1] = rotation_matrix[4];
		camera_x_axis[2] = rotation_matrix[8];

		float camera_y_axis[3];
		camera_y_axis[0] = rotation_matrix[1];
		camera_y_axis[1] = rotation_matrix[5];
		camera_y_axis[2] = rotation_matrix[9];

		float camera_z_axis[3];
		camera_z_axis[0] = rotation_matrix[2];
		camera_z_axis[1] = rotation_matrix[6];
		camera_z_axis[2] = rotation_matrix[10];

		float offset_x[3];
		vec3_mul_s(camera_x_axis, offset[0], offset_x);
		
		float offset_y[3];
		vec3_mul_s(camera_y_axis, offset[1], offset_y);
		
		float offset_z[3];
		vec3_mul_s(camera_z_axis, offset[2], offset_z);

		vec3_add(ed.camera.position, offset_x, ed.camera.position);
		vec3_add(ed.camera.position, offset_y, ed.camera.position);
		vec3_add(ed.camera.position, offset_z, ed.camera.position);
	}
	
	float translation_matrix[16];
	matrix_make_translation(
		-ed.camera.position[0],
		-ed.camera.position[1],
		-ed.camera.position[2],
		translation_matrix);

	matrix_multiply(rotation_matrix, translation_matrix, ed.camera.view_matrix);

	// keep track of the entity that the mouse is pointing at
	if (CX_OBJECT_ID_GET_CATEGORY(ed.object_id_at_cursor) == CX_OBJECT_ID_CATEGORY_ENTITY) {
		ed.entity_id_at_cursor = (uint16_t)CX_OBJECT_ID_GET_PAYLOAD(ed.object_id_at_cursor);
	} else {
		ed.entity_id_at_cursor = CX_ENTITY_ID_INVALID;
	}

	if (ed.gizmo.interaction_state != CX_TRANSFORM_GIZMO_INTERACTION_STATE_in_progress &&
		input_frame_is_mouse_button_released(MOUSE_BUTTON_left)) {
		
		ed.selected_entity_id = ed.entity_id_at_cursor;
	}

	// upate the gizmo while we have a selected entity
	if (ed.selected_entity_id != CX_ENTITY_ID_INVALID) {
		int mouse_client_coords[2];
		platform_window_get_mouse_client_coords(ed.p_window, &mouse_client_coords[0], &mouse_client_coords[1]);

		float projection_view_matrix[16];
		matrix_multiply(ed.camera.projection_matrix, ed.camera.view_matrix, projection_view_matrix);

		float cursor_ray[3];
		platform_window_client_to_world_ray(ed.p_window,
			projection_view_matrix,
			mouse_client_coords[0], mouse_client_coords[1],
			cursor_ray);

		const float gizmo_view_scale = 2.0f / ed.camera.projection_matrix[5];
		
		struct transform* p_selected_entity_transform =
			cx_world_entity_get_transform(&ed.world, ed.selected_entity_id);

		struct transform t;
		const enum cx_transform_gizmo_interaction_state gizmo_interaction_state = 
			cx_transform_gizmo_update(
				&ed.gizmo,
				p_selected_entity_transform,
				ed.object_id_at_cursor,
				ed.camera.position, gizmo_view_scale, cursor_ray,
				&t);

		if (gizmo_interaction_state == CX_TRANSFORM_GIZMO_INTERACTION_STATE_in_progress) {
			*p_selected_entity_transform = t;
		} else if (gizmo_interaction_state == CX_TRANSFORM_GIZMO_INTERACTION_STATE_ended) {
			CX_ACTION_EXECUTE(set_entity_transform, &((struct cx_ed_action_set_entity_transform_ctx) {
				.p_world = &ed.world,
				.entity_id = ed.selected_entity_id,
				.transform_old = ed.gizmo.drag_state.initial_target_transform,
				.transform = t
			}));
		} else {
			if (input_frame_is_key_pressed(KEY_e)) {
				ed.gizmo.mode = CX_TRANSFORM_GIZMO_MODE_translate;
			} else if (input_frame_is_key_pressed(KEY_r)) {
				ed.gizmo.mode = CX_TRANSFORM_GIZMO_MODE_rotate;
			} else if (input_frame_is_key_pressed(KEY_t)) {
				ed.gizmo.mode = CX_TRANSFORM_GIZMO_MODE_scale;
			}
		}
	}

	cx_world_compute_transforms(&ed.world);
}

void cx_ed_world_editor_draw(const struct cx_gfx_framebuffer* p_fb, uint32_t fb_width, uint32_t fb_height) {
	matrix_make_perspective_projection(
		1,
		(float)fb_width / (float)fb_height,
		0.01f, 1000.0f,
		ed.camera.projection_matrix);

	struct cx_render_draw_command draw_commands[1024];
	struct cx_render_draw_command_buffer draw_command_buffer = {
		.p_first = draw_commands,
		.capacity = CX_ARRAY_LEN(draw_commands)
	};
		   
	// draw scene
	{
		struct cx_gfx_shader_program_input_block render_pass_shader_program_input_block = {
			.s_name = "blk_camera",
			.size = sizeof(ed.camera.projection_matrix) + sizeof(ed.camera.view_matrix),
			.p_data = ed.camera.projection_matrix
		};

		struct cx_gfx_render_pass render_pass_world = {
			.p_framebuffer = p_fb,
			.viewport = { 0, 0, (int32_t)fb_width, (int32_t)fb_height },
			.clear_flags =
				CX_GFX_RENDER_TARGET_CLEAR_FLAG_color |
				CX_GFX_RENDER_TARGET_CLEAR_FLAG_depth,
			.clear_color = { 0.2f, 0.2f, 0.2f, 0.0f },
			.clear_depth = 1.0f,
			.pass_input_set = {
				.p_blocks = &render_pass_shader_program_input_block,
				.num_blocks = 1
			}
		};

		cx_world_renderer_record_draw_commands_lit(&ed.world, &draw_command_buffer);

		cx_gfx_render_pass_execute(&render_pass_world, draw_command_buffer.p_first, draw_command_buffer.len);

		draw_command_buffer.len = 0;
	}

	// draw grid
	{
		struct {
			float view_projection_matrix[16];
			float inv_view_projection_matrix[16];
		} camera_block;

		matrix_multiply(ed.camera.projection_matrix, ed.camera.view_matrix, camera_block.view_projection_matrix);
		matrix_inverse(4, camera_block.view_projection_matrix, camera_block.inv_view_projection_matrix);

		struct cx_gfx_shader_program_input_block render_pass_shader_program_input_block = {
			.s_name = "blk_camera",
			.size = sizeof(camera_block),
			.p_data = &camera_block
		};

		struct cx_gfx_render_pass render_pass_world = {
			.p_framebuffer = p_fb,
			.viewport = { 0, 0, (int32_t)fb_width, (int32_t)fb_height },
			.pass_input_set = {
				.p_blocks = &render_pass_shader_program_input_block,
				.num_blocks = 1
			}
		};

		cx_render_draw_command_buffer_push(&draw_command_buffer, &(struct cx_render_draw_command) {
			.pipeline = ed.render_pipeline_editor_grid,
			.p_mesh = &ed.dummy_mesh
		});

		cx_gfx_render_pass_execute(&render_pass_world, draw_command_buffer.p_first, draw_command_buffer.len);

		draw_command_buffer.len = 0;
	}

	// gizmo
	if (ed.selected_entity_id != CX_ENTITY_ID_INVALID) {
		struct cx_gfx_shader_program_input_block render_pass_shader_program_input_block = {
			.s_name = "blk_camera",
			.size = sizeof(ed.camera.projection_matrix) + sizeof(ed.camera.view_matrix),
			.p_data = ed.camera.projection_matrix
		};

		struct cx_gfx_render_pass render_pass_world = {
			.p_framebuffer = p_fb,
			.viewport = { 0, 0, (int32_t)fb_width, (int32_t)fb_height },
			.clear_flags = CX_GFX_RENDER_TARGET_CLEAR_FLAG_depth,
			.clear_depth = 1.0f,
			.pass_input_set = {
				.p_blocks = &render_pass_shader_program_input_block,
				.num_blocks = 1
			}
		};

		cx_transform_gizmo_record_draw_commands(&ed.gizmo, &ed.render_pipeline_flat, &draw_command_buffer);
		
		cx_gfx_render_pass_execute(&render_pass_world, draw_command_buffer.p_first, draw_command_buffer.len);

		draw_command_buffer.len = 0;
	}

	// object ids
	{
		cx_object_id_capturer_set_fb_size(&ed.object_id_capturer, fb_width, fb_height);

		struct cx_gfx_shader_program_input_block render_pass_shader_program_input_block = {
			.s_name = "blk_camera",
			.size = sizeof(ed.camera.projection_matrix) + sizeof(ed.camera.view_matrix),
			.p_data = ed.camera.projection_matrix
		};

		struct cx_gfx_render_pass render_pass_world = {
			.p_framebuffer = &ed.object_id_capturer.framebuffer,
			.viewport = {
				0,
				0, 
				(int32_t)ed.object_id_capturer.framebuffer_width,
				(int32_t)ed.object_id_capturer.framebuffer_height
			},
			.clear_flags =
				CX_GFX_RENDER_TARGET_CLEAR_FLAG_color |
				CX_GFX_RENDER_TARGET_CLEAR_FLAG_depth,
			.clear_depth = 1.0f,
			.pass_input_set = {
				.p_blocks = &render_pass_shader_program_input_block,
				.num_blocks = 1
			}
		};

		cx_world_renderer_record_draw_commands_object_id(&ed.world, &draw_command_buffer);

		if (ed.selected_entity_id != CX_ENTITY_ID_INVALID) {
			cx_transform_gizmo_record_draw_commands(&ed.gizmo, &ed.render_pipeline_object_id, &draw_command_buffer);
		}

		cx_gfx_render_pass_execute(&render_pass_world, draw_command_buffer.p_first, draw_command_buffer.len);

		draw_command_buffer.len = 0;
	}

	int mouse_client_coords[2];
	platform_window_get_mouse_client_coords(ed.p_window, &mouse_client_coords[0], &mouse_client_coords[1]);

	float mouse_position_normalized[2];
	platform_window_normalize_client_coords(ed.p_window,
		mouse_client_coords[0], mouse_client_coords[1],
		&mouse_position_normalized[0], &mouse_position_normalized[1]);

	ed.object_id_at_cursor = cx_object_id_capturer_query(&ed.object_id_capturer,
		mouse_position_normalized[0], mouse_position_normalized[1]);

	CX_LAZYLOG_FMT("object_id_at_cursor=%u\n", CX_OBJECT_ID_GET_PAYLOAD(ed.object_id_at_cursor));
}

void cx_ed_world_editor_on_key(const void* p_e, void* p_user_ptr) {
	(void)p_user_ptr;

	const struct input_event_data_key* p_key_event = p_e;

	if (!p_key_event->b_is_down) {
		return;
	}

	if (p_key_event->key == KEY_z && p_key_event->mods & INPUT_MOD_ctrl) {
		cx_ed_action_history_undo(&ed.action_history);
	}
	else if (p_key_event->key == KEY_y && p_key_event->mods & INPUT_MOD_ctrl) {
		cx_ed_action_history_redo(&ed.action_history);
	}
}

cx_result cx_ed_world_editor_load_world_from_world_blueprint(const char* s_asset_name) {
	if (cx_asset_ref_is_set(&ed.world_blueprint_asset_ref)) {
		cx_ed_asset_library_save(ed.world_blueprint_asset_ref.asset_id);
		cx_asset_cache_release(&ed.world_blueprint_asset_ref);
	}

	if (!cx_asset_cache_find_by_name(CX_ASSET_TYPE_WORLD_BLUEPRINT, s_asset_name, &ed.world_blueprint_asset_ref)) {
		struct cx_world_blueprint* p_world_blueprint =
			CX_MALLOC(cx_asset_type_size(CX_ASSET_TYPE_WORLD_BLUEPRINT));

		*p_world_blueprint = (struct cx_world_blueprint){0};
		
		cx_ed_asset_library_new(
			CX_ASSET_TYPE_WORLD_BLUEPRINT,
			s_asset_name,
			p_world_blueprint,
			&ed.world_blueprint_asset_ref);
	}

	struct cx_world_blueprint* p_world_bp = cx_asset_cache_acquire(&ed.world_blueprint_asset_ref);

	CX_LOG_FMT(INFO, ED_WORLD_EDITOR, "Loaded world from blueprint %X\n", ed.world_blueprint_asset_ref.asset_id);

	cx_world_instantiate_blueprint(&ed.world, &p_world_bp->root);
	
	return CX_SUCCESS;
}

int cx_cmd_world_editor_open_world_blueprint(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	(void)p_context;

	const char* s_asset_name = p_args->list->as_str.p;

	return cx_ed_world_editor_load_world_from_world_blueprint(s_asset_name);
}

cx_result cx_ed_world_editor_save_world_to_world_blueprint(const char* s_save_as_filepath) {
	CX_ASSERT(ed.world_blueprint_asset_ref.asset_id != 0, ED_WORLD_EDITOR);

	struct cx_world_blueprint* p_world_blueprint = cx_asset_cache_acquire(&ed.world_blueprint_asset_ref);
	struct cx_blueprint* p_bp = &p_world_blueprint->root;

	cx_blueprint_destroy(&p_world_blueprint->root);

	uint16_t entity_node_map[CX_WORLD_MAX_ENTITIES];

	for (uint16_t i = 0; i < CX_WORLD_MAX_ENTITIES; ++i) {
		if (!ed.world.entities[i].b_alive) {
			continue;
		}

		uint16_t node_id = cx_blueprint_create_node(p_bp);
		entity_node_map[i] = node_id;

		struct transform* p_transform = cx_blueprint_node_get_transform(&p_world_blueprint->root, node_id);
		*p_transform = ed.world.entities[i].transform;
		p_transform->p_local_transform = CX_NULL;

		CX_LOG_FMT(INFO, ED_WORLD_EDITOR, "Saving entity to world blueprint: id=%u, node_id=%u\n", i, node_id);

		for (uint16_t j = 0; j < ed.world.num_component_pools; ++j) {
			const struct cx_component_pool* p_pool = &ed.world.p_component_pools[j];
			const uint16_t dense_index = p_pool->sparse[i];
			if (dense_index >= p_pool->count || p_pool->p_dense_entities[dense_index] != i) {
				continue;
			}

			CX_LOG_FMT(INFO, ED_WORLD_EDITOR, "  Saving component data: type='%s', size=%"CX_PRI_SIZE"\n",
				p_pool->p_type->s_name, p_pool->p_type->size);

			const size_t component_data_off = p_pool->p_type->size * dense_index;
			const uint8_t* p_component_data = p_pool->p_dense_components + component_data_off;

			void* p_node_component_data = cx_blueprint_node_add_component(p_bp, node_id, p_pool->p_type);
	
			memcpy(p_node_component_data, p_component_data, p_pool->p_type->size);
		}
	}

	// Set up parent-child node relationships

	for (uint16_t i = 0; i < CX_WORLD_MAX_ENTITIES; ++i) {
		if (!ed.world.entities[i].b_alive) {
			continue;
		}

		for (uint16_t j = 0; j < CX_WORLD_MAX_ENTITIES; ++j) {
			if (i == j || !ed.world.entities[i].b_alive) {
				continue;
			}

			if (ed.world.entities[i].transform.p_local_transform != &ed.world.entities[j].transform) {
				continue;
			}

			CX_LOG_FMT(
				INFO, 
				ED_WORLD_EDITOR,
				"Saving entity local transform: id=%u, parent_id=%u, node_id=%u, parent_node_id=%u\n",
				i, j, entity_node_map[i], entity_node_map[j]);

			cx_blueprint_node_set_parent(p_bp, entity_node_map[i], entity_node_map[j]);
		}
	}

	cx_result result;

	if (s_save_as_filepath) {
		result = cx_ed_asset_library_save_as(ed.world_blueprint_asset_ref.asset_id, s_save_as_filepath);
	} else {
		result = cx_ed_asset_library_save(ed.world_blueprint_asset_ref.asset_id);
	}

	cx_asset_cache_release(&ed.world_blueprint_asset_ref);

	return result;
}

int cx_cmd_world_editor_save_world_blueprint(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	(void)p_context;

	const char* s_save_as_filepath = p_args->count > 0 ? p_args->list[0].as_str.p : CX_NULL;
	cx_result result = cx_ed_world_editor_save_world_to_world_blueprint(s_save_as_filepath);

	char flog_str_buf[128];
	struct cx_flog_builder flog = {
		.p_buf = flog_str_buf,
	};

	if (result == CX_ERROR_ASSET_NO_FILEPATH) {
		cx_flog_append(&flog, "Asset requires a save file location\n");
		cx_flog_end(p_context->p_flogger, &flog);
	} else if (result == CX_ERROR_ALREADY_EXISTS) {
		cx_flog_append(&flog, "File already exists\n");
		cx_flog_end(p_context->p_flogger, &flog);
	}


	return result;
}

int cx_cmd_world_editor_spawn_blueprint(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	(void)p_context;

	struct cx_asset_ref blueprint_asset_ref;
	if (!cx_asset_cache_find_by_name(CX_ASSET_TYPE_BLUEPRINT, p_args->list[0].as_str.p, &blueprint_asset_ref)) {
		return 1;
	}

	const struct cx_blueprint* p_blueprint = cx_asset_cache_acquire(&blueprint_asset_ref);

	if (!p_blueprint) {
		return 2;
	}

	cx_world_instantiate_blueprint(&ed.world, p_blueprint);

	cx_asset_cache_release(&blueprint_asset_ref);

	return 0;
}

cx_result cx_ed_world_editor_entity_new(uint16_t* p_out_entity_id) {
	*p_out_entity_id = cx_world_entity_create(&ed.world);
	return CX_SUCCESS;
}

int cx_cmd_world_editor_entity_new(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	
	(void)p_args;

	uint16_t new_entity_id;
	cx_ed_world_editor_entity_new(&new_entity_id);

	char flog_buf[128];

	struct cx_flog_builder flog_builder = {
		.p_buf = flog_buf
	};

	cx_flog_append_fmt(&flog_builder, "New entity created: id=%u", new_entity_id);
	cx_flog_end(p_context->p_flogger, &flog_builder);

	return CX_SUCCESS;
}

cx_result cx_ed_world_editor_entity_destroy(uint16_t entity_id) {
	cx_world_entity_destroy(&ed.world, entity_id);
	return CX_SUCCESS;
}

static uint16_t cx_ed_world_editor_get_entity_id_from_cmd_arg(
	const union cx_var_value* p_arg, struct cx_flogger* p_flogger, char* p_flog_buf) {

	struct cx_flog_builder flog_builer = { .p_buf = p_flog_buf };

	const char* p_arg_str = p_arg->as_str.p;

	if (cx_strncmp(p_arg_str, "@", p_arg->as_str.len) == 0) {
		if (ed.selected_entity_id == CX_ENTITY_ID_INVALID) {
			cx_flog_append(&flog_builer, "No entity selected");
			cx_flog_end(p_flogger, &flog_builer);
		}
		return ed.selected_entity_id;
	}

	char* p;
	unsigned long num = strtoul(p_arg_str, &p, 10);

	if ((size_t)(p - p_arg_str) < p_arg->as_str.len ||
		num >= CX_WORLD_MAX_ENTITIES ||
		!cx_world_entity_is_alive(&ed.world, (uint16_t)num)) {

		cx_flog_append_fmt(&flog_builer, "Invalid entity id: %u", num);
		cx_flog_end(p_flogger, &flog_builer);

		return CX_ENTITY_ID_INVALID;
	}

	return (uint16_t)num;
}

int cx_cmd_world_editor_entity_destroy(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	char flog_buf[128];

	uint16_t entity_id =
		cx_ed_world_editor_get_entity_id_from_cmd_arg(&p_args->list[0], p_context->p_flogger, flog_buf);

	if (entity_id == CX_ENTITY_ID_INVALID) {
		return CX_ERROR_NOT_FOUND;
	}

	cx_ed_world_editor_entity_destroy(entity_id);

	return CX_SUCCESS;
}

cx_result cx_ed_world_editor_entity_clone(uint16_t entity_id) {
	return CX_SUCCESS;
}

int cx_cmd_world_editor_entity_clone(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	return CX_SUCCESS;
}

cx_result cx_ed_world_editor_entity_set_parent(uint16_t entity_id, uint16_t parent_entity_id) {
	return CX_SUCCESS;
}

int cx_cmd_world_editor_entity_set_parent(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	return CX_SUCCESS;
}

cx_result cx_ed_world_editor_entity_add_component(uint16_t entity_id, const struct cx_component_type* p_type) {
	if (cx_world_component_add(&ed.world, entity_id, p_type)) {
		return CX_SUCCESS;
	}
	return CX_ERROR_ALREADY_EXISTS;
}

int cx_cmd_world_editor_entity_add_component(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	char flog_buf[128];

	uint16_t entity_id =
		cx_ed_world_editor_get_entity_id_from_cmd_arg(&p_args->list[0], p_context->p_flogger, flog_buf);

	if (entity_id == CX_ENTITY_ID_INVALID) {
		return CX_ERROR_NOT_FOUND;
	}

	struct cx_flog_builder flog_builder = { .p_buf = flog_buf };

	const struct cx_component_type* p_type;
	if (!cx_component_find_type(p_args->list[1].as_str.p, &p_type)) {
		cx_flog_append_fmt(&flog_builder, "Component type '%s' not found", p_args->list[1].as_str.p);
		cx_flog_end(p_context->p_flogger, &flog_builder);
		return CX_ERROR_NOT_FOUND;
	}
	
	if (cx_world_component_has(&ed.world, entity_id, p_type)) {
		cx_flog_append_fmt(&flog_builder, "Entity already has '%s' component", p_args->list[1].as_str.p);
		cx_flog_end(p_context->p_flogger, &flog_builder);
		return CX_ERROR_INVALID_ARG;
	}

	cx_flog_append_fmt(&flog_builder, "Component '%s' added", p_args->list[1].as_str.p);
	cx_flog_end(p_context->p_flogger, &flog_builder);

	return cx_ed_world_editor_entity_add_component(entity_id, p_type);
}

cx_result cx_ed_world_editor_entity_remove_component(uint16_t entity_id, const struct cx_component_type* p_type) {
	cx_world_component_remove(&ed.world, entity_id, p_type);
	return CX_SUCCESS;
}

int cx_cmd_world_editor_entity_remove_component(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	char flog_buf[128];

	uint16_t entity_id =
		cx_ed_world_editor_get_entity_id_from_cmd_arg(&p_args->list[0], p_context->p_flogger, flog_buf);

	if (entity_id == CX_ENTITY_ID_INVALID) {
		return CX_ERROR_NOT_FOUND;
	}

	struct cx_flog_builder flog_builder = { .p_buf = flog_buf };

	const struct cx_component_type* p_type;
	if (!cx_component_find_type(p_args->list[1].as_str.p, &p_type)) {
		cx_flog_append_fmt(&flog_builder, "Component type '%s' not found", p_args->list[1].as_str.p);
		cx_flog_end(p_context->p_flogger, &flog_builder);
		return CX_ERROR_NOT_FOUND;
	}
	
	if (!cx_world_component_has(&ed.world, entity_id, p_type)) {
		cx_flog_append_fmt(&flog_builder, "Entity doesn't have a '%s' component", p_args->list[1].as_str.p);
		cx_flog_end(p_context->p_flogger, &flog_builder);
		return CX_ERROR_INVALID_ARG;
	}

	cx_flog_append_fmt(&flog_builder, "Component '%s' removed", p_args->list[1].as_str.p);
	cx_flog_end(p_context->p_flogger, &flog_builder);

	return cx_ed_world_editor_entity_remove_component(entity_id, p_type);
}
