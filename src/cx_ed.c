#include "cx_asset.h"
#include "cx_cmp_collider.h"
#include "cx_cmp_rigidbody.h"
#include "cx_cmp_static_mesh.h"
#include "cx_command.h"
#include "cx_command_registry.h"
#include "cx_console.h"
#include "cx_ed.h"
#include "cx_ed_action.h"
#include "cx_ed_import_gltf.h"
#include "cx_ed_transform_gizmo.h"
#include "cx_io.h"
#include "cx_macro.h"
#include "cx_object_id_capturer.h"
#include "cx_var.h"
#include "cx_world.h"
#include "cx_world_renderer.h"
#include "input.h"
#include "matrix.h"
#include "physics.h"
#include "platform_window.h"
#include "vector.h"

#include "gl.h"

#define CX_ACTION_DEF(NAME)\
	static void cx_ed_action_##NAME##_do(void* p_ctx);\
	static void cx_ed_action_##NAME##_undo(void* p_ctx);\
	static const struct cx_ed_action_def action_def_##NAME = {\
		.context_size = sizeof(struct cx_ed_action_##NAME##_ctx),\
		.f_do = cx_ed_action_##NAME##_do,\
		.f_undo = cx_ed_action_##NAME##_undo\
	}

#define CX_ACTION_EXECUTE(NAME, P_CTX)\
	cx_ed_action_history_execute(&ed.action_history, &action_def_##NAME, P_CTX)

static struct {
	struct platform_window* p_window;

	struct cx_asset_package asset_package;

	struct cx_ed_action_history action_history;

	struct cx_render_command render_commands[CX_WORLD_MAX_ENTITIES];
	struct cx_render_pass render_pass_forward;
	struct cx_render_pass render_pass_flat_color;

	struct cx_world world;
	struct physics_world physics_world;

	struct cx_object_id_capturer object_id_capturer;

	uint32_t object_id_at_cursor;
	uint16_t selected_entity_id;
	uint16_t entity_id_at_cursor;

	struct cx_transform_gizmo gizmo;

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

// CREATE ENTITY

uint16_t cx_ed_create_entity(float x, float y, float z);
int cx_ed_create_entity_command(const struct cx_command_args* p_args, const struct cx_command_context* p_ctx);

struct cx_ed_action_create_entity_ctx {
	struct cx_world* p_world;
	float position[3];
	uint16_t entity_id;
};

CX_ACTION_DEF(create_entity);

void cx_ed_action_create_entity_do(void* p_ctx) {
	struct cx_ed_action_create_entity_ctx* p_c = p_ctx;
	p_c->entity_id = cx_world_entity_create(p_c->p_world);
	struct transform* p_t = cx_world_entity_get_transform(p_c->p_world, p_c->entity_id);
	transform_set_world_position(p_t, p_c->position);
}

void cx_ed_action_create_entity_undo(void* p_ctx) {
	struct cx_ed_action_create_entity_ctx* p_c = p_ctx;
	cx_world_entity_destroy(p_c->p_world, p_c->entity_id);
}

int cx_ed_create_entity_command(const struct cx_command_args* p_args, const struct cx_command_context* p_ctx) {
	(void)p_ctx;
	const double x = p_args->count > 0 ? p_args->list[0].as_float : 0;
	const double y = p_args->count > 1 ? p_args->list[1].as_float : 0;
	const double z = p_args->count > 2 ? p_args->list[2].as_float : 0;
	
	const uint16_t new_entity_id = cx_ed_create_entity((float)x, (float)y, (float)z);

	struct cx_flog_builder flog = ed.flog_builder;

	cx_flog_append_fmt(&flog, "Created new entity (%d) at [%g, %g, %g]\n", new_entity_id, x, y, z);
	cx_flog_end(p_ctx->p_flogger, &flog);
	
	return 1;
}

uint16_t cx_ed_create_entity(float x, float y, float z) {
	struct cx_ed_action_create_entity_ctx ctx = {
		.p_world = &ed.world,
		.position = { x , y, z }
	};

	CX_ACTION_EXECUTE(create_entity, &ctx);

	return ctx.entity_id;
}

uint16_t cx_ed_destroy_entity(uint16_t entity_id);
uint16_t cx_ed_set_entity_transform(uint16_t entity_id);
uint16_t cx_ed_set_entity_parent(uint16_t entity_id, uint16_t parent_entity_id);

static void cx_ed_on_key(const void* p_e, void* p_user_ptr);

void cx_ed_update(double dt_seconds) {
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
		if (input_frame_is_key_down(KEY_ctrl_left)) {
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

	if (input_frame_is_mouse_button_released(MOUSE_BUTTON_left)) {
		ed.selected_entity_id = ed.entity_id_at_cursor;
	}

	// upate the gizmo while we have a selected entity
	if (ed.selected_entity_id != CX_ENTITY_ID_INVALID) {
		struct transform* p_selected_entity_transform =
			cx_world_entity_get_transform(&ed.world, ed.selected_entity_id);

		int mouse_client_coords[2];
		platform_window_get_mouse_client_coords(ed.p_window, &mouse_client_coords[0], &mouse_client_coords[1]);

		float cursor_ray[3];
		platform_window_client_to_world_ray(ed.p_window,
			ed.camera.projection_matrix,
			mouse_client_coords[0], mouse_client_coords[1],
			cursor_ray);
		
		const float gizmo_view_scale = 2.0f / ed.camera.projection_matrix[5];

		struct transform t;
		const enum cx_transform_gizmo_interaction_state gizmo_interaction_state = 
			cx_transform_gizmo_update(
				&ed.gizmo,
				p_selected_entity_transform,
				ed.object_id_at_cursor,
				ed.camera.position, gizmo_view_scale, cursor_ray,
				&t);

		if (gizmo_interaction_state == CX_TRANSFORM_GIZMO_INTERACTION_STATE_in_progress) {
			struct transform* p_t = cx_world_entity_get_transform(&ed.world, ed.selected_entity_id);
			*p_t = t;
		} else if (gizmo_interaction_state == CX_TRANSFORM_GIZMO_INTERACTION_STATE_ended) {
			struct transform* p_t = cx_world_entity_get_transform(&ed.world, ed.selected_entity_id);
			*p_t = t;
			// todo: execute history action
		}
	}

	cx_world_compute_transforms(&ed.world);
}

void cx_ed_draw(const struct cx_gfx_framebuffer* p_fb, uint32_t fb_width, uint32_t fb_height) {
	matrix_make_perspective_projection(
		1,
		(float)fb_width / (float)fb_height,
		0.01f, 1000.0f,
		ed.camera.projection_matrix);
            
	struct cx_render_pass_execute_info render_pass_execute_info = {
		.p_framebuffer = p_fb,
		.viewport = { 0, 0, (int32_t)fb_width, (int32_t)fb_height },
		.b_clear_color = 1,
		.clear_color = { 0.2f, 0.2f, 0.2f, 0.0f },
		.b_clear_depth = 1,
		.clear_depth = 1.0f
	};

	struct cx_render_command_buffer render_command_buffer = {
		.p_commands = ed.render_commands,
		.capacity = CX_ARRAY_LEN(ed.render_commands)
	};

	struct cx_render_pass_data render_pass_data = {
		.p_data = &ed.camera.projection_matrix[0]
	};

	cx_world_renderer_record_forward_pass_commands(&ed.world, &render_command_buffer);

	cx_render_pass_execute(
		&ed.render_pass_forward,
		&render_pass_execute_info,
		&render_pass_data,
		&render_command_buffer);
	render_command_buffer.num = 0;

	if (ed.selected_entity_id != CX_ENTITY_ID_INVALID) {
		cx_transform_gizmo_record_flat_color_pass_commands(&ed.gizmo, &render_command_buffer);

		render_pass_execute_info.b_clear_color = 0;

		cx_render_pass_execute(
			&ed.render_pass_flat_color,
			&render_pass_execute_info,
			&render_pass_data,
			&render_command_buffer);
		render_command_buffer.num = 0;
	}

	cx_world_renderer_record_picker_pass_commands(&ed.world, &render_command_buffer);

	if (ed.selected_entity_id != CX_ENTITY_ID_INVALID) {
		cx_transform_gizmo_record_picker_pass_commands(&ed.gizmo, &render_command_buffer);
	}

	cx_object_id_capturer_draw(
		&ed.object_id_capturer,
		ed.camera.projection_matrix,
		ed.camera.view_matrix, fb_width, fb_height,
		&render_command_buffer);
	render_command_buffer.num = 0;

	int mouse_client_coords[2];
	platform_window_get_mouse_client_coords(ed.p_window, &mouse_client_coords[0], &mouse_client_coords[1]);

	float mouse_position_normalized[2];
	platform_window_normalize_client_coords(ed.p_window,
		mouse_client_coords[0], mouse_client_coords[1],
		&mouse_position_normalized[0], &mouse_position_normalized[1]);

	ed.object_id_at_cursor = cx_object_id_capturer_query(&ed.object_id_capturer,
		mouse_position_normalized[0], mouse_position_normalized[1]);
}

void cx_ed_init(struct platform_window* p_window) {
	ed.p_window = p_window;

	cx_asset_package_init(&ed.asset_package);

	ed.flog_builder = (struct cx_flog_builder) {
		.p_buf = ed.flog_builder_str_buf,
		.p_styles = ed.flog_builder_style_buf,
		.p_spans = ed.flog_builder_span_buf
	};

	ed.entity_id_at_cursor = CX_ENTITY_ID_INVALID;
	ed.selected_entity_id = CX_ENTITY_ID_INVALID;

	CX_NEW_COMMAND("ent.create", "Create a new entity", cx_ed_create_entity_command, 0,
		CX_COMMAND_PARAM(FLOAT("x", "Spawn position X"), OPTIONAL),
		CX_COMMAND_PARAM(FLOAT("y", "Spawn position Y"), OPTIONAL),
		CX_COMMAND_PARAM(FLOAT("z", "Spawn position Z"), OPTIONAL));
	
	void* p_vsource;
	void* p_fsource;
	
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/lit.vert", (void**)&p_vsource, 0) == CX_ERROR_none, ED);
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/lit.frag", (void**)&p_fsource, 0) == CX_ERROR_none, ED);

	CX_ASSERT(cx_render_pass_build(&((struct cx_render_pass_build_info){
		.program_source = {
			.s_vertex_stage_source = p_vsource,
			.s_fragment_stage_source = p_fsource
		},
		.s_pass_block_name = "blk_camera",
		.s_object_block_name = "blk_object",
		.s_material_block_name = "blk_material_properties",
		.p_s_opaque_param_names = (const char*[]){ "u_texture_albedo" },
		.num_opaque_params = 1
	}), &ed.render_pass_forward), ED);

	cx_io_file_free(p_vsource);
	cx_io_file_free(p_fsource);

	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/flat.vert", (void**)&p_vsource, 0) == CX_ERROR_none, ED);
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/flat.frag", (void**)&p_fsource, 0) == CX_ERROR_none, ED);

	CX_ASSERT(cx_render_pass_build(&((struct cx_render_pass_build_info){
		.program_source = {
			.s_vertex_stage_source = p_vsource,
			.s_fragment_stage_source = p_fsource
		},
		.s_pass_block_name = "blk_camera",
		.s_object_block_name = "blk_object",
		.s_material_block_name = "blk_material_properties",
	}), &ed.render_pass_flat_color), ED);

	cx_io_file_free(p_vsource);
	cx_io_file_free(p_fsource);

	cx_transform_gizmo_init_shared_resources(&ed.asset_package);
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

	struct cx_asset_package_record* p_gltf_scene_blueprint_asset;
	cx_ed_import_gltf_file(&ed.asset_package, "res/Industrial_exterior_v2.glb", &p_gltf_scene_blueprint_asset);
	struct cx_blueprint* p_gltf_scene_blueprint = p_gltf_scene_blueprint_asset->asset_.p_data_;

	cx_world_instantiate_blueprint(&ed.world, p_gltf_scene_blueprint);

	input_event_subscribe(INPUT_EVENT_key, cx_ed_on_key, 0);
}

void cx_ed_on_key(const void* p_e, void* p_user_ptr) {
	(void)p_user_ptr;

	const struct input_event_data_key* p_key_event = p_e;

	if (p_key_event->key == KEY_v && p_key_event->mods & INPUT_MOD_ctrl) {
		
	}
}
