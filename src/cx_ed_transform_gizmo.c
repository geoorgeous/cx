#include "cx_asset.h"
#include "cx_blueprint.h"
#include "cx_cmp_static_mesh.h"
#include "cx_ed_import_gltf.h"
#include "cx_ed_transform_gizmo.h"
#include "cx_gfx_mesh.h"
#include "cx_logging.h"
#include "cx_object_id_capturer.h"
#include "input.h"
#include "matrix.h"
#include "mouse_buttons.h"
#include "static_mesh.h"
#include "vector.h"

#define CX_TRANSFORM_GIZMO_OBJECT_ID_CATEGORY 2

#define CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X      0
#define CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y      1
#define CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z      2
#define CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XY     3
#define CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XZ     4
#define CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_YZ     5
#define CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER 6

#define CX_TRANSFORM_GIZMO_CONTROL_COLOR_X      ((const float[]){ 0.961f, 0.306f, 0.306f, 1.000f })
#define CX_TRANSFORM_GIZMO_CONTROL_COLOR_Y      ((const float[]){ 0.525f, 0.941f, 0.090f, 1.000f })
#define CX_TRANSFORM_GIZMO_CONTROL_COLOR_Z      ((const float[]){ 0.243f, 0.478f, 1.000f, 1.000f })
#define CX_TRANSFORM_GIZMO_CONTROL_COLOR_CENTER ((const float[]){ 0.800f, 0.800f, 0.800f, 1.000f })
#define CX_TRANSFORM_GIZMO_CONTROL_COLOR_HOVER  ((const float[]){ 0.900f, 0.800f, 0.300f, 1.000f })

#define CX_TRANSFORM_GIZMO_X_AXIS ((const float[]){ 1, 0, 0 })
#define CX_TRANSFORM_GIZMO_Y_AXIS ((const float[]){ 0, 1, 0 })
#define CX_TRANSFORM_GIZMO_Z_AXIS ((const float[]){ 0, 0, 1 })

static struct {
	const struct cx_gfx_mesh* t_meshes[7];
	const struct cx_gfx_mesh* r_meshes[4];
	const struct cx_gfx_mesh* s_meshes[7];
} shared_resources;

static void cx_transform_gizmo_update_transform(
	struct cx_transform_gizmo* p_gizmo,
	const struct transform* p_target_transform,
	const float* p_view_pos,
	float gizmo_view_scale);

static void cx_transform_gizmo_apply(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	struct transform* p_out_transform);

static void cx_transform_gizmo_apply_translation(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	const float* p_v, float* p_out_v);

static void cx_transform_gizmo_apply_rotation(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	const float* p_q, float* p_out_q);

static void cx_transform_gizmo_apply_scale(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	const float* p_q, float* p_out_q);

static int cx_transform_gizmo_compute_control_plane_cursor_intersection(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray,
	float* p_cursor_world_pos);

static void cx_transform_gizmo_compute_control_plane_normal(
	const float* p_control_axis,
	const float* p_view_pos,
	float* p_plane_norm_d);

static void cx_transform_gizmo_compute_cursor_delta_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	float* p_cursor_world_delta);

static float cx_transform_gizmo_compute_cursor_angle_delta_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start);

static void cx_transform_gizmo_compute_cursor_delta_on_axis(
	const float* p_control_origin, const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	float* p_cursor_world_delta);

static void cx_transform_gizmo_apply_translation_on_axis(
	const float* p_control_origin, const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v);

static void cx_transform_gizmo_apply_translation_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v);

static void cx_transform_gizmo_apply_rotation_around_axis(
	const float* p_control_origin, const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_q, float* p_out_q);

static void cx_transform_gizmo_apply_rotation_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_q, float* p_out_q);

static void cx_transform_gizmo_apply_scale_on_axis(
	const float* p_constol_origin, const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v);

static void cx_transform_gizmo_apply_scale_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v);

static void cx_transform_gizmo_apply_scale_uniformly(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v);

static inline void cx_transform_gizmo_init_shared_resource(
	const struct cx_blueprint* p_blueprint,
	size_t bp_node_index,
	const struct cx_gfx_mesh** pp_out_mesh) {

	struct cx_cmp_static_mesh* p_cmp_static_mesh =
		cx_blueprint_node_find_component(p_blueprint, p_blueprint->p_nodes[bp_node_index].id, &cmp_type_static_mesh);

	struct static_mesh* p_static_mesh = p_cmp_static_mesh->p_asset_package_record->asset_.p_data_;
	static_mesh_load_device_meshes(p_static_mesh);

	*pp_out_mesh = &p_static_mesh->p_gfx_meshes[0];
}

static void cx_transform_gizmo_compute_control_drag_plane_normal(
	enum cx_transform_gizmo_mode mode,
	int control,
	const float* p_gizmo_transform,
	const float* p_view_pos,
	const float* p_cursor_world_ray,
	float* p_out_normal) {

	switch (mode) {
		case CX_TRANSFORM_GIZMO_MODE_translate: {
		switch (control) {
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X: {
				vec3_norm(&p_gizmo_transform[0], p_out_normal);
				cx_transform_gizmo_compute_control_plane_normal(p_out_normal, p_view_pos, p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y: {
				vec3_norm(&p_gizmo_transform[4], p_out_normal);
				cx_transform_gizmo_compute_control_plane_normal(p_out_normal, p_view_pos, p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z: {
				vec3_norm(&p_gizmo_transform[8], p_out_normal);
				cx_transform_gizmo_compute_control_plane_normal(p_out_normal, p_view_pos, p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XY: {
				vec3_norm(&p_gizmo_transform[8], p_out_normal);
				break;
			}
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XZ: {
				vec3_norm(&p_gizmo_transform[4], p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_YZ: {
				vec3_norm(&p_gizmo_transform[0], p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER: {
				vec3_mul_s(p_cursor_world_ray, -1, p_out_normal);
				break;
			}
		} }

		case CX_TRANSFORM_GIZMO_MODE_rotate: {
		switch (control) {
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X: {
				vec3_norm(&p_gizmo_transform[0], p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y: {
				vec3_norm(&p_gizmo_transform[4], p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z: {
				vec3_norm(&p_gizmo_transform[8], p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER: {
				vec3_mul_s(p_cursor_world_ray, -1, p_out_normal);
				break;
			}
		} }

		case CX_TRANSFORM_GIZMO_MODE_scale: {
		switch(control) {
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_YZ:
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X: {
				vec3_copy(CX_TRANSFORM_GIZMO_X_AXIS, p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XZ:
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y: {
				vec3_copy(CX_TRANSFORM_GIZMO_Y_AXIS, p_out_normal);
				break;
			}

			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XY:
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z: {
				vec3_copy(CX_TRANSFORM_GIZMO_Z_AXIS, p_out_normal);
				break;
			}
			
			case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER: {
				float control_plane_normal[3];
				vec3_mul_s(p_cursor_world_ray, -1, control_plane_normal);
				break;
			}
		} }
    }
}

void cx_transform_gizmo_init_shared_resources(struct cx_asset_package* p_package) {
	struct cx_asset_package_record* p_gltf_scene_blueprint_asset;
	struct cx_blueprint* p_blueprint;

	cx_ed_import_gltf_file(p_package, "res/builtin/gizmo_translate.glb", &p_gltf_scene_blueprint_asset);
	p_blueprint = p_gltf_scene_blueprint_asset->asset_.p_data_;

	cx_transform_gizmo_init_shared_resource(p_blueprint, 5, &shared_resources.t_meshes[0]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 6, &shared_resources.t_meshes[1]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 4, &shared_resources.t_meshes[2]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 2, &shared_resources.t_meshes[3]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 3, &shared_resources.t_meshes[4]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 0, &shared_resources.t_meshes[5]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 1, &shared_resources.t_meshes[6]);

	cx_ed_import_gltf_file(p_package, "res/builtin/gizmo_rotate.glb", &p_gltf_scene_blueprint_asset);
	p_blueprint = p_gltf_scene_blueprint_asset->asset_.p_data_;

	cx_transform_gizmo_init_shared_resource(p_blueprint, 1, &shared_resources.r_meshes[0]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 2, &shared_resources.r_meshes[1]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 0, &shared_resources.r_meshes[2]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 3, &shared_resources.r_meshes[3]);

	cx_ed_import_gltf_file(p_package, "res/builtin/gizmo_scale.glb", &p_gltf_scene_blueprint_asset);
	p_blueprint = p_gltf_scene_blueprint_asset->asset_.p_data_;

	cx_transform_gizmo_init_shared_resource(p_blueprint, 3, &shared_resources.s_meshes[0]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 4, &shared_resources.s_meshes[1]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 2, &shared_resources.s_meshes[2]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 1, &shared_resources.s_meshes[3]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 5, &shared_resources.s_meshes[4]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 6, &shared_resources.s_meshes[5]);
	cx_transform_gizmo_init_shared_resource(p_blueprint, 0, &shared_resources.s_meshes[6]);
}

#undef CX_GIZMO_CONTROL_INIT

static inline void cx_transform_gizmo_init_control(
	uint32_t object_id,
	const float* p_color,
	struct cx_transform_gizmo_control_render_data* p_out) {

	matrix_make_identity(p_out->object_data.transform);
	p_out->object_data.object_id = CX_OBJECT_ID_MAKE(CX_TRANSFORM_GIZMO_OBJECT_ID_CATEGORY, object_id);
	vec3_copy(p_color, p_out->material_data.color_ka);
}

void cx_transform_gizmo_init_controls(struct cx_transform_gizmo* p_gizmo) {
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_X, &p_gizmo->render_data.t[0]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Y, &p_gizmo->render_data.t[1]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Z, &p_gizmo->render_data.t[2]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XY,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Z, &p_gizmo->render_data.t[3]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XZ,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Y, &p_gizmo->render_data.t[4]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_YZ,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_X, &p_gizmo->render_data.t[5]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_CENTER, &p_gizmo->render_data.t[6]);

	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_X, &p_gizmo->render_data.r[0]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Y, &p_gizmo->render_data.r[1]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Z, &p_gizmo->render_data.r[2]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_CENTER, &p_gizmo->render_data.r[3]);

	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_X, &p_gizmo->render_data.s[0]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Y, &p_gizmo->render_data.s[1]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Z, &p_gizmo->render_data.s[2]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XY,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Z, &p_gizmo->render_data.s[3]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XZ,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_Y, &p_gizmo->render_data.s[4]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_YZ,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_X, &p_gizmo->render_data.s[5]);
	cx_transform_gizmo_init_control(
		CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER,
		CX_TRANSFORM_GIZMO_CONTROL_COLOR_CENTER, &p_gizmo->render_data.s[6]);
}

enum cx_transform_gizmo_interaction_state cx_transform_gizmo_update(
	struct cx_transform_gizmo* p_gizmo,
	const struct transform* p_target_transform,
	uint32_t object_id_under_cursor,
	const float* p_view_pos,
	float gizmo_view_scale,
	const float* p_cursor_world_ray,
	struct transform* p_out_transform) {

	cx_transform_gizmo_update_transform(p_gizmo, p_target_transform, p_view_pos, gizmo_view_scale);

	if (p_gizmo->interaction_state == CX_TRANSFORM_GIZMO_INTERACTION_STATE_idle) {
		const int b_mouse_on_gizmo =
			CX_OBJECT_ID_GET_CATEGORY(object_id_under_cursor) ==
			CX_TRANSFORM_GIZMO_OBJECT_ID_CATEGORY;

		if (input_frame_is_mouse_button_pressed(MOUSE_BUTTON_left) && b_mouse_on_gizmo) {
			p_gizmo->drag_state.initial_target_transform = *p_target_transform;

			float control_drag_plane_normal[3];
			cx_transform_gizmo_compute_control_drag_plane_normal(
				p_gizmo->mode,
				CX_OBJECT_ID_GET_PAYLOAD(object_id_under_cursor),
				p_gizmo->gizmo_transform,
				p_view_pos,
				p_cursor_world_ray,
				control_drag_plane_normal);

			cx_transform_gizmo_compute_control_plane_cursor_intersection(
				&p_gizmo->gizmo_transform[12],
				control_drag_plane_normal,
				p_view_pos,
				p_cursor_world_ray,
				p_gizmo->drag_state.manipulation_origin);
			
			p_gizmo->interaction_state = CX_TRANSFORM_GIZMO_INTERACTION_STATE_in_progress;

			CX_LOG(INFO, GIZMO, "interaction started\n");

		}
	} else if (p_gizmo->interaction_state == CX_TRANSFORM_GIZMO_INTERACTION_STATE_in_progress) {
		cx_transform_gizmo_apply(
			p_gizmo,
			CX_OBJECT_ID_GET_PAYLOAD(object_id_under_cursor),
			p_view_pos, p_cursor_world_ray,
			p_out_transform);

		cx_transform_gizmo_update_transform(p_gizmo, p_out_transform, p_view_pos, gizmo_view_scale);

		if (input_frame_is_mouse_button_released(MOUSE_BUTTON_left)) {
			p_gizmo->interaction_state = CX_TRANSFORM_GIZMO_INTERACTION_STATE_ended;

			CX_LOG(INFO, GIZMO, "interaction ended\n");
		}
	} else {
		p_gizmo->interaction_state = CX_TRANSFORM_GIZMO_INTERACTION_STATE_idle;
	}

	return p_gizmo->interaction_state;
}

void cx_transform_gizmo_record_flat_color_pass_commands(
	const struct cx_transform_gizmo* p_gizmo,
	struct cx_render_command_buffer* p_buffer) {

	switch (p_gizmo->mode) {
		case CX_TRANSFORM_GIZMO_MODE_translate: {
			for (size_t i = 0; i < 7; ++i) {
				cx_render_command_buffer_push(p_buffer, &((struct cx_render_command){
					.p_mesh = shared_resources.t_meshes[i],
					.p_object_data = &p_gizmo->render_data.t[i].object_data,
					.p_material_data = &p_gizmo->render_data.t[i].material_data,
				}));
			}
			break;
		}

		case CX_TRANSFORM_GIZMO_MODE_rotate: {
			for (size_t i = 0; i < 4; ++i) {
				cx_render_command_buffer_push(p_buffer, &((struct cx_render_command){
					.p_mesh = shared_resources.r_meshes[i],
					.p_object_data = &p_gizmo->render_data.r[i].object_data,
					.p_material_data = &p_gizmo->render_data.r[i].material_data,
				}));
			}
			break;
		}

		case CX_TRANSFORM_GIZMO_MODE_scale: {
			for (size_t i = 0; i < 7; ++i) {
				cx_render_command_buffer_push(p_buffer, &((struct cx_render_command){
					.p_mesh = shared_resources.s_meshes[i],
					.p_object_data = &p_gizmo->render_data.s[i].object_data,
					.p_material_data = &p_gizmo->render_data.s[i].material_data,
				}));
			}
			break;
		}
	}
}

void cx_transform_gizmo_record_picker_pass_commands(
	const struct cx_transform_gizmo* p_gizmo,
	struct cx_render_command_buffer* p_buffer) {

	cx_transform_gizmo_record_flat_color_pass_commands(p_gizmo, p_buffer);
}

void cx_transform_gizmo_update_transform(
	struct cx_transform_gizmo* p_gizmo,
	const struct transform* p_target_transform,
	const float* p_view_pos,
	float gizmo_view_scale) {
	
	const float gizmo_viewport_size = 0.15f;
    
    float object_to_camera[3];
    vec3_sub(p_view_pos, p_target_transform->world_position, object_to_camera);
    float scale = gizmo_view_scale * vec3_len(object_to_camera) * gizmo_viewport_size;
    
    matrix_make_uniform_scale(scale, p_gizmo->gizmo_transform);
    
	// transform gizmo does not support non-local scaling/skewering
    if (p_gizmo->b_use_local_axes || p_gizmo->mode == CX_TRANSFORM_GIZMO_MODE_scale) {
        float local_rotation[16];
        matrix_make_rotation_from_quaternion(p_target_transform->world_rotation, local_rotation);
        matrix_multiply(local_rotation, p_gizmo->gizmo_transform, p_gizmo->gizmo_transform);
    }

    float translation[16];
    matrix_make_translation(
		p_target_transform->world_position[0],
		p_target_transform->world_position[1],
		p_target_transform->world_position[2], translation);

    matrix_multiply(translation, p_gizmo->gizmo_transform, p_gizmo->gizmo_transform);

	// update per-control transform
	for (size_t i = 0; i < 7; ++i) {
		matrix_copy(p_gizmo->gizmo_transform, p_gizmo->render_data.t[i].object_data.transform);
		matrix_copy(p_gizmo->gizmo_transform, p_gizmo->render_data.s[i].object_data.transform);
	}
	for (size_t i = 0; i < 4; ++i) {
		matrix_copy(p_gizmo->gizmo_transform, p_gizmo->render_data.r[i].object_data.transform);
	}
}

void cx_transform_gizmo_apply(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	struct transform* p_out_transform) {

	switch (p_gizmo->mode) {
		case CX_TRANSFORM_GIZMO_MODE_translate: {
			cx_transform_gizmo_apply_translation(p_gizmo,
				control_id,
				p_view_pos, p_view_ray,
				p_out_transform->world_position, p_out_transform->world_position);
			break;
		}

		case CX_TRANSFORM_GIZMO_MODE_rotate: {
			cx_transform_gizmo_apply_rotation(p_gizmo, 
				control_id,
				p_view_pos, p_view_ray,
				p_out_transform->world_rotation, p_out_transform->world_rotation);
			break;
		}

		case CX_TRANSFORM_GIZMO_MODE_scale: {
			float new_world_scale[3];
			cx_transform_gizmo_apply_scale(p_gizmo,
				control_id,
				p_view_pos, p_view_ray,
				p_out_transform->world_scale, new_world_scale);
			transform_set_world_scale(p_out_transform, new_world_scale);
			break;
		}
	}

}

void cx_transform_gizmo_apply_translation(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	const float* p_v, float* p_out_v) {

    switch (control_id) {
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X: {
            float control_axis[3];
            vec3_norm(&p_gizmo->gizmo_transform[0], control_axis);
            cx_transform_gizmo_apply_translation_on_axis(
				&p_gizmo->gizmo_transform[12], control_axis,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y: {
            float control_axis[3];
            vec3_norm(&p_gizmo->gizmo_transform[4], control_axis);
            cx_transform_gizmo_apply_translation_on_axis(
				&p_gizmo->gizmo_transform[12], control_axis,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z: {
            float control_axis[3];
            vec3_norm(&p_gizmo->gizmo_transform[8], control_axis);
            cx_transform_gizmo_apply_translation_on_axis(
				&p_gizmo->gizmo_transform[12], control_axis,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XY: {
            float control_plane_normal[3];
            vec3_norm(&p_gizmo->gizmo_transform[8], control_plane_normal);
            cx_transform_gizmo_apply_translation_on_plane(
				&p_gizmo->gizmo_transform[12], control_plane_normal,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XZ: {
            float control_plane_normal[3];
            vec3_norm(&p_gizmo->gizmo_transform[4], control_plane_normal);
            cx_transform_gizmo_apply_translation_on_plane(
				&p_gizmo->gizmo_transform[12], control_plane_normal,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_YZ: {
            float control_plane_normal[3];
            vec3_norm(&p_gizmo->gizmo_transform[0], control_plane_normal);
            cx_transform_gizmo_apply_translation_on_plane(
				&p_gizmo->gizmo_transform[12], control_plane_normal,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER: {
            float control_plane_normal[3];
            vec3_mul_s(p_view_ray, -1, control_plane_normal);
            cx_transform_gizmo_apply_translation_on_plane(
				&p_gizmo->gizmo_transform[12], control_plane_normal,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
    }
}

void cx_transform_gizmo_apply_rotation(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	const float* p_q, float* p_out_q) {
    switch (control_id) {
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X: {
            float control_axis[3];
            vec3_norm(&p_gizmo->gizmo_transform[0], control_axis);
            cx_transform_gizmo_apply_rotation_around_axis(
				&p_gizmo->gizmo_transform[12], control_axis,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_q, p_out_q);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y: {
            float control_axis[3];
            vec3_norm(&p_gizmo->gizmo_transform[4], control_axis);
            cx_transform_gizmo_apply_rotation_around_axis(
				&p_gizmo->gizmo_transform[12], control_axis,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_q, p_out_q);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z: {
            float control_axis[3];
            vec3_norm(&p_gizmo->gizmo_transform[8], control_axis);
            cx_transform_gizmo_apply_rotation_around_axis(
				&p_gizmo->gizmo_transform[12], control_axis,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_q, p_out_q);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER: {
            float control_plane_normal[3];
            vec3_mul_s(p_view_ray, -1, control_plane_normal);
            cx_transform_gizmo_apply_rotation_on_plane(
				&p_gizmo->gizmo_transform[12], control_plane_normal,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_q, p_out_q);
            break;
        }
    }
}

void cx_transform_gizmo_apply_scale(
	const struct cx_transform_gizmo* p_gizmo,
	uint32_t control_id,
	const float* p_view_pos, const float* p_view_ray,
	const float* p_v, float* p_out_v) {

    switch (control_id) {
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_X: {
            cx_transform_gizmo_apply_scale_on_axis(
				&p_gizmo->gizmo_transform[12], CX_TRANSFORM_GIZMO_X_AXIS,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Y: {
            cx_transform_gizmo_apply_scale_on_axis(
				&p_gizmo->gizmo_transform[12], CX_TRANSFORM_GIZMO_Y_AXIS,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }

        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_Z: {
            cx_transform_gizmo_apply_scale_on_axis(
				&p_gizmo->gizmo_transform[12], CX_TRANSFORM_GIZMO_Z_AXIS,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XY: {
            cx_transform_gizmo_apply_scale_on_plane(
				&p_gizmo->gizmo_transform[12], CX_TRANSFORM_GIZMO_Z_AXIS,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
		case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_XZ: {
            cx_transform_gizmo_apply_scale_on_plane(
				&p_gizmo->gizmo_transform[12], CX_TRANSFORM_GIZMO_Y_AXIS,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_YZ: {
            cx_transform_gizmo_apply_scale_on_plane(
				&p_gizmo->gizmo_transform[12], CX_TRANSFORM_GIZMO_X_AXIS,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
        
        case CX_TRANSFORM_GIZMO_CONTROL_OBJECT_ID_CENTER: {
            float control_plane_normal[3];
            vec3_mul_s(p_view_ray, -1, control_plane_normal);
            cx_transform_gizmo_apply_scale_uniformly(
				&p_gizmo->gizmo_transform[12], control_plane_normal,
				p_view_pos, p_view_ray,
				p_gizmo->drag_state.manipulation_origin,
				p_v, p_out_v);
            break;
        }
    }
}

int cx_transform_gizmo_compute_control_plane_cursor_intersection(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray,
	float* p_cursor_world_pos) {

    float control_plane_d = -vec3_dot(p_control_plane_normal, p_control_origin);

    if (!ray_plane_intersection(
		p_cursor_ray_origin,
		p_cursor_ray,
		p_control_plane_normal,
		control_plane_d,
		p_cursor_world_pos)) {

        return 0;
    }

	return 1;
}

void cx_transform_gizmo_compute_control_plane_normal(
	const float* p_control_axis, const float* p_view_pos, float* p_plane_norm_d) {

    float side[3];
    float up[3];
    compute_compliment_axes(p_control_axis, side, up);
    
    float view_dir[3];
    vec3_inv(p_view_pos, view_dir);
    vec3_norm(view_dir, view_dir);

    // Select the plane with the best viewing angle
    if (fabsf(vec3_dot(side, view_dir)) < fabsf(vec3_dot(up, view_dir))) {
        vec3_copy(up, p_plane_norm_d);
    } else {
        vec3_copy(side, p_plane_norm_d);
    }
}

void cx_transform_gizmo_compute_cursor_delta_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	float* p_out_cursor_world_delta) {

    float cursor_ray_intersection[3];
    if (!cx_transform_gizmo_compute_control_plane_cursor_intersection(
		p_control_origin,
		p_control_plane_normal,
		p_cursor_ray_origin,
		p_cursor_ray,
		cursor_ray_intersection)) {

        vec3_clr(p_out_cursor_world_delta);
        return;
    }

    vec3_sub(cursor_ray_intersection, p_cursor_world_start, p_out_cursor_world_delta);
}

float cx_transform_gizmo_compute_cursor_angle_delta_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start) {

    float p0[3];
    float p1[3];

    if (!cx_transform_gizmo_compute_control_plane_cursor_intersection(
		p_control_origin,
		p_control_plane_normal,
		p_cursor_ray_origin,
		p_cursor_ray,
		p1)) {

        return 0;
    }

    vec3_sub(p_cursor_world_start, p_control_origin, p0);

    vec3_sub(p1, p_control_origin, p1);
    
    float cross[3];
    vec3_cross(p0, p1, cross);
    
    return atan2f(vec3_dot(p_control_plane_normal, cross), vec3_dot(p0, p1));
}

void cx_transform_gizmo_compute_cursor_delta_on_axis(
	const float* p_control_origin,
	const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	float* p_cursor_world_delta) {

    float n[3];
    vec3_norm(p_control_axis, n);

    float plane_norm[3];
    cx_transform_gizmo_compute_control_plane_normal(n, p_cursor_ray_origin, plane_norm);

    cx_transform_gizmo_compute_cursor_delta_on_plane(
		p_control_origin,
		plane_norm,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start,
		p_cursor_world_delta);

    float dot = vec3_dot(p_cursor_world_delta, n);

    vec3_mul_s(n, dot, p_cursor_world_delta);
}

void cx_transform_gizmo_apply_translation_on_axis(
	const float* p_control_origin, const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v) {

    float cursor_world_delta[3];
    cx_transform_gizmo_compute_cursor_delta_on_axis(
		p_control_origin,
		p_control_axis,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start,
		cursor_world_delta);

    vec3_add(p_v, cursor_world_delta, p_out_v);
}

void cx_transform_gizmo_apply_translation_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_v_out) {

    float cursor_world_delta[3];
    cx_transform_gizmo_compute_cursor_delta_on_plane(
		p_control_origin,
		p_control_plane_normal,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start,
		cursor_world_delta);
    
    vec3_add(p_v, cursor_world_delta, p_v_out);
}

void cx_transform_gizmo_apply_rotation_around_axis(
	const float* p_control_origin, const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_q, float* p_out_q) {

    const float angle = cx_transform_gizmo_compute_cursor_angle_delta_on_plane(
		p_control_origin,
		p_control_axis,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start);

    if (FLT_CMP(angle, 0)) {
        return;
    }

    float rotation[4];
    quaternion_from_axis_angle(p_control_axis, angle, rotation);
    quaternion_multiply(rotation, p_q, p_out_q);
    quaternion_norm(p_out_q, p_out_q);
}

void cx_transform_gizmo_apply_rotation_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_q, float* p_out_q) {

    float cursor_world_delta[3];
    cx_transform_gizmo_compute_cursor_delta_on_plane(
		p_control_origin,
		p_control_plane_normal,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start,
		cursor_world_delta);

    float side[3];
    float up[3];
    compute_compliment_axes(p_control_plane_normal, side, up);

    const float angle_side = vec3_dot(up, cursor_world_delta);
    float rotation_side[4];
    quaternion_from_axis_angle(side, angle_side, rotation_side);

    const float angle_up = -vec3_dot(side, cursor_world_delta);
    float rotation_up[4];
    quaternion_from_axis_angle(up, angle_up, rotation_up);
    
    float rotation[4];
    quaternion_multiply(rotation_side, rotation_up, rotation);
    quaternion_norm(rotation, rotation);
    
    quaternion_multiply(rotation, p_q, p_out_q);
    quaternion_norm(p_out_q, p_out_q);
}

void cx_transform_gizmo_apply_scale_on_axis(
	const float* p_control_origin, const float* p_control_axis,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v) {

    float cursor_world_delta[3];
    cx_transform_gizmo_compute_cursor_delta_on_axis(
		p_control_origin,
		p_control_axis,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start,
		cursor_world_delta);
    
    const float mod = 0.3f;
    vec3_mul_s(cursor_world_delta, mod, cursor_world_delta);
    
    vec3_add(p_v, cursor_world_delta, p_out_v);
}

void cx_transform_gizmo_apply_scale_on_plane(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v) {

    float cursor_world_delta[3];
    cx_transform_gizmo_compute_cursor_delta_on_plane(
		p_control_origin,
		p_control_plane_normal,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start,
		cursor_world_delta);

    const float mod = 0.3f;
    vec3_mul_s(cursor_world_delta, mod, cursor_world_delta);

    vec3_add(p_v, cursor_world_delta, p_out_v);
}

void cx_transform_gizmo_apply_scale_uniformly(
	const float* p_control_origin, const float* p_control_plane_normal,
	const float* p_cursor_ray_origin, const float* p_cursor_ray, const float* p_cursor_world_start,
	const float* p_v, float* p_out_v) {

    float cursor_world_delta[3];
    cx_transform_gizmo_compute_cursor_delta_on_plane(
		p_control_origin,
		p_control_plane_normal,
		p_cursor_ray_origin,
		p_cursor_ray,
		p_cursor_world_start,
		cursor_world_delta);

    const float avg = (cursor_world_delta[0] + cursor_world_delta[1] + cursor_world_delta[2]) / 3;
    const float mod = 0.3f;

    vec3_add_s(p_v, avg * mod, p_out_v);
}
