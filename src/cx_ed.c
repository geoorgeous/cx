#include "cx_asset.h"
#include "cx_cmp_collider.h"
#include "cx_cmp_rigidbody.h"
#include "cx_cmp_static_mesh.h"
#include "cx_command.h"
#include "cx_command_registry.h"
#include "cx_console.h"
#include "cx_ed.h"
#include "cx_ed_import_gltf.h"
#include "cx_macro.h"
#include "cx_var.h"
#include "cx_world.h"
#include "cx_world_renderer.h"
#include "input.h"
#include "matrix.h"
#include "vector.h"

#define CX_ACTION_DEF(NAME)\
	static void cx_ed_action_##NAME##_do(void* p_ctx);\
	static void cx_ed_action_##NAME##_undo(void* p_ctx);\
	const static struct cx_ed_action_def action_def_##NAME = {\
		.context_size = sizeof(struct cx_ed_action_##NAME##_ctx),\
		.f_do = cx_ed_action_##NAME##_do,\
		.f_undo = cx_ed_action_##NAME##_undo\
	}

#define CX_ACTION_EXECUTE(NAME, P_CTX)\
	cx_ed_action_history_execute(&ed.action_history, &action_def_##NAME, P_CTX)

static struct {
	struct cx_asset_package asset_package;

	struct cx_ed_action_history action_history;

	struct cx_world world;

	// buffers for command flog builder
	char                   flog_builder_str_buf[1024];
	struct cx_flog_style   flog_builder_style_buf[8];
	struct cx_flog_span    flog_builder_span_buf[8];
	struct cx_flog_builder flog_builder;

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
	uint16_t entity_id;
	float position[3];
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
	float x = p_args->count > 0 ? p_args->list[0].as_float : 0;
	float y = p_args->count > 1 ? p_args->list[1].as_float : 0;
	float z = p_args->count > 2 ? p_args->list[2].as_float : 0;
	
	const uint16_t new_entity_id = cx_ed_create_entity(x, y, z);

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

			ed.camera.pitch += mouse_delta_y * 0.01f;
			ed.camera.yaw += mouse_delta_x * 0.01f;
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
		vec3_mul_s(move_direction, speed * dt_seconds, offset);

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
}

void cx_ed_draw(float aspect) {
	matrix_make_perspective_projection(
		1,
		aspect,
		0.01f, 1000.0f,
		ed.camera.projection_matrix);

	cx_world_renderer_draw(&ed.world, ed.camera.projection_matrix, ed.camera.view_matrix);
}

void cx_ed_init(void) {
	cx_asset_package_init(&ed.asset_package);

	ed.flog_builder = (struct cx_flog_builder) {
		.p_buf = ed.flog_builder_str_buf,
		.p_styles = ed.flog_builder_style_buf,
		.p_spans = ed.flog_builder_span_buf
	};

	CX_NEW_COMMAND("ent.create", "Create a new entity", cx_ed_create_entity_command, 0,
		CX_COMMAND_PARAM(FLOAT("x", "Entity world position, x component"), OPTIONAL),
		CX_COMMAND_PARAM(FLOAT("y", "Entity world position, y component"), OPTIONAL),
		CX_COMMAND_PARAM(FLOAT("z", "Entity world position, z component"), OPTIONAL));

	struct cx_component_pool_def world_component_pool_defs[] = {
		{ &cmp_type_static_mesh,  CX_WORLD_MAX_ENTITIES },
		{ &cmp_type_collider,     512 },
		{ &cmp_type_rigidbody,    128 }
	};

	cx_world_init(&ed.world, world_component_pool_defs, CX_ARRAY_LEN(world_component_pool_defs));

	struct cx_asset_package_record* p_gltf_scene_blueprint_asset;
	cx_ed_import_gltf_file(&ed.asset_package, "res/Industrial_exterior_v2.glb", &p_gltf_scene_blueprint_asset);
	struct cx_blueprint* p_gltf_scene_blueprint = p_gltf_scene_blueprint_asset->asset_.p_data_;

	cx_world_instantiate_blueprint(&ed.world, p_gltf_scene_blueprint);
}
