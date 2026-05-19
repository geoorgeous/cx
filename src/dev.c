#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "asset.h"
#include "cx_commands.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"
#include "cx_logging.h"
#include "cx_mesh_gen.h"
#include "dev.h"
#include "gltf.h"
#include "half_edge.h"
#include "hashtable.h"
#include "import_gltf.h"
#include "input.h"
#include "math_utils.h"
#include "matrix.h"
#include "mesh.h"
#include "mesh_id_capturer.h"
#include "physics.h"
#include "platform_window.h"
#include "quickhull.h"
#include "scene.h"
#include "static_mesh.h"
#include "transform.h"
#include "vector.h"

#define DEV_MESH_ID_CAPTURER_ID_GIZMO_T_X      1
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_T_Y      2
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_T_Z      3
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_T_XY     4
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_T_XZ     5
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_T_YZ     6
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_T_CENTER 7
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_R_X      8
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_R_Y      9
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_R_Z      10
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_R_CENTER 11
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_S_X      12
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_S_Y      13
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_S_Z      14
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_S_XY     15
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_S_XZ     16
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_S_YZ     17
#define DEV_MESH_ID_CAPTURER_ID_GIZMO_S_CENTER 18  

#define DEV_MESH_ID_CAPTURER_RESERVED_IDS      32

enum gizmo_type {
    GIZMO_TYPE_translate,
    GIZMO_TYPE_rotate,
    GIZMO_TYPE_scale
};

struct gizmo_control {
    struct cx_gfx_mesh gfx_mesh;
    unsigned int       mesh_id_capturer_id;
    float              color_ka[4];
};

static struct dev_state {
    int                           b_dev_mode_enabled;

    const struct platform_window* p_window;
    struct scene*                 p_scene;
    struct scene_entity*          p_selected_entity;
    struct mesh_id_capturer       mesh_id_capturer;
    unsigned int                  target_mesh_id;
    unsigned int                  pressed_mesh_id;

    int                           b_is_dragging;

    struct cx_gfx_program              program_flat;
	struct cx_gfx_program_param_block  program_pblock_camera;
	struct cx_gfx_program_param_block  program_pblock_object;
	struct cx_gfx_program_param_block  program_pblock_material;
	struct cx_gfx_program_param_buffer program_pbuffer_camera;
	struct cx_gfx_program_param_buffer program_pbuffer_object;
	struct cx_gfx_program_param_buffer program_pbuffer_material;
    
    struct asset_package asset_package;

    float perspective_view_matrix[16];
    float perspective_scale;
    float camera_position[3];

    struct physics_world* p_physics_world;
    int                   b_draw_physics;
	struct cx_gfx_mesh    physics_collider_mesh_sphere;
	struct cx_gfx_mesh    physics_collider_mesh_capsule_cap;
	struct cx_gfx_mesh    physics_collider_mesh_capsule_mid;
	struct cx_gfx_mesh    physics_collider_mesh_plane;
	struct hashtable      physics_hull_meshes;
    float*                p_hull_points;
    size_t                num_hull_points;

    struct gizmos_state {
        enum gizmo_type      active_type;
        int                  b_use_local_axes;
        float                gizmo_transform[16];
        struct transform*    p_target_transform;
        float                cursor_drag_last_world_pos[3];
        float                hovered_control_color_ka[4];
        struct gizmo_control control_t_x;
        struct gizmo_control control_t_y;
        struct gizmo_control control_t_z;
        struct gizmo_control control_t_xy;
        struct gizmo_control control_t_xz;
        struct gizmo_control control_t_yz;
        struct gizmo_control control_t_center;
        struct gizmo_control control_r_x;
        struct gizmo_control control_r_y;
        struct gizmo_control control_r_z;
        struct gizmo_control control_r_center;
        struct gizmo_control control_s_x;
        struct gizmo_control control_s_y;
        struct gizmo_control control_s_z;
        struct gizmo_control control_s_xy;
        struct gizmo_control control_s_xz;
        struct gizmo_control control_s_yz;
        struct gizmo_control control_s_center;
    } gizmos;

} g_dev;

static void on_key(const void* p_event_data, void* p_user_ptr);
static void on_mouse_button(const void* p_event_data, void* p_user_ptr);
static void on_mouse_move(const void* p_event_data, void* p_user_ptr);

static void set_selected_entity(struct scene_entity* p_entity);

static void toggle_draw_physics_colliders(void);
static void draw_physics(void);
static void compute_physics_collider_mesh_trs_sphere(const struct scene_entity* p_entity, float* p_out_trs);
static void compute_physics_collider_mesh_trs_capsule(
	const struct scene_entity* p_entity,
	float* p_out_trs_mid, float* p_out_trs_cap_p0, float* p_out_trs_cap_p1);
static void compute_physics_collider_mesh_trs_plane(const struct scene_entity* p_entity, float* p_out_trs);

static void  draw_gizmo(void);
static void  draw_gizmo_control(const struct gizmo_control* p_control);
static int   find_gizmo_control_plane_cursor_drag_intersection(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_pos);
static void  compute_gizmo_control_axis_drag_plane_normal(const float* p_control_axis, const float* p_view_pos, float* p_plane_norm_d);
static void  compute_gizmo_control_plane_cursor_drag_delta(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_delta);
static float compute_gizmo_control_plane_cursor_drag_delta_angle(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray);
static void  compute_gizmo_control_axis_cursor_drag_delta(const float* p_control_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_delta);
static void  gizmo_drag_translate_axis(const float* p_control_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray);
static void  gizmo_drag_translate_plane(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray);
static void  gizmo_drag_rotate(const float* p_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray);
static void  gizmo_drag_rotate_ball(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray);
static void  gizmo_drag_scale_axis(const float* p_control_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray);
static void  gizmo_drag_scale_plane(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray);
static void  gizmo_drag_scale_uniformly(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray);

static void send_scene_to_mesh_id_capturer(void);
static void send_gizmo_to_mesh_id_capturer(void);
static void send_gizmo_control_to_mesh_id_capturer(const struct gizmo_control* p_control);

static void draw_hull_DEBUGDEBUGDEBUG(void);

static void cmd_toggle_draw_physics_colliders(const struct cx_command_context* p_context);

void dev_mode_enable(void) {
	CX_DBG(CX_LOG(INFO, DEV, "Dev mode enabled\n"));
    g_dev.b_dev_mode_enabled = 1;
}

void dev_mode_disable(void) {
	CX_DBG(CX_LOG(INFO, DEV, "Dev mode disabled\n"));
    g_dev.b_dev_mode_enabled = 0;
}

int dev_mode_is_enabled(void) {
    return g_dev.b_dev_mode_enabled;
}

void dev_init(const struct platform_window* p_window, struct scene* p_scene, struct physics_world* p_physics_world) {
    g_dev.p_window = p_window;

    g_dev.p_scene = p_scene;

    g_dev.p_physics_world = p_physics_world;

	g_dev.mesh_id_capturer = (struct mesh_id_capturer){0};

    input_event_subscribe(INPUT_EVENT_key, on_key, 0);
    input_event_subscribe(INPUT_EVENT_mouse_button, on_mouse_button, 0);
    input_event_subscribe(INPUT_EVENT_mouse_move, on_mouse_move, 0);

	struct cx_gfx_program_source program_source = {
		.s_vertex_stage_source = "#version 330 core\n"
			"layout(std140) uniform blk_camera {"
				"mat4 u_projection_matrix;"
				"mat4 u_view_matrix;"
			"};"
			"layout(std140) uniform blk_object {"
				"mat4 u_vertex_matrix;"
			"};"
			"layout (location=0) in vec3 a_pos;"
			"layout (location=1) in vec3 a_normal;"
			"out vec3 v_normal;"
			"void main() {"
				"v_normal = normalize(mat3(transpose(inverse(u_vertex_matrix))) * a_normal);"
				"gl_Position = u_projection_matrix * u_view_matrix * u_vertex_matrix * vec4(a_pos, 1.0);"
			"}",
		.s_fragment_stage_source = "#version 330 core\n"
			"layout(std140) uniform blk_material_properties {"
				"vec3  u_color;"
				"float u_ka;"
			"};"
			"in vec3 v_normal;"
			"out vec4 f_color;"
			"void main() {"
				"const vec3 light_dir = normalize(-vec3(1, 2, 1));"
				"float kd = max(dot(v_normal, -light_dir), 0);"
				"f_color = vec4(min((u_ka + kd), 1) * u_color, 1);"
			"}"
	};

	cx_gfx_program_create(&g_dev.program_flat);
	cx_gfx_program_build(&g_dev.program_flat, &program_source);

	cx_gfx_program_refl_param_block(&g_dev.program_flat, "blk_camera", &g_dev.program_pblock_camera);
	cx_gfx_program_refl_param_block(&g_dev.program_flat, "blk_object", &g_dev.program_pblock_object);
	cx_gfx_program_refl_param_block(&g_dev.program_flat, "blk_material_properties", &g_dev.program_pblock_material);

	cx_gfx_program_param_buffer_create(&g_dev.program_pbuffer_camera, g_dev.program_pblock_camera.size_);
	cx_gfx_program_param_buffer_create(&g_dev.program_pbuffer_object, g_dev.program_pblock_object.size_);
	cx_gfx_program_param_buffer_create(&g_dev.program_pbuffer_material, g_dev.program_pblock_material.size_);

    asset_package_init(&g_dev.asset_package);

    // load gizmos
    
    g_dev.gizmos.active_type = GIZMO_TYPE_translate;

    struct gltf gltf;
    struct import_gltf_result import_result;

    gltf_load_from_file("res/builtin/gizmo_translate.glb", &gltf);
    import_gltf(&gltf, &g_dev.asset_package, &import_result);
    
    cx_gfx_mesh_create(&g_dev.gizmos.control_t_x.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[5]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_t_y.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[6]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_t_z.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[4]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_t_xy.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[2]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_t_xz.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[3]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_t_yz.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[0]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_t_center.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[1]->asset_.p_data_))->p_primitives[0]);
    
    gltf_free(&gltf);
    import_gltf_free(&import_result);

    gltf_load_from_file("res/builtin/gizmo_rotate.glb", &gltf);
    import_gltf(&gltf, &g_dev.asset_package, &import_result);
    
    cx_gfx_mesh_create(&g_dev.gizmos.control_r_x.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[1]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_r_y.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[2]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_r_z.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[0]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_r_center.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[3]->asset_.p_data_))->p_primitives[0]);
    
    gltf_free(&gltf);
    import_gltf_free(&import_result);

    gltf_load_from_file("res/builtin/gizmo_scale.glb", &gltf);
    import_gltf(&gltf, &g_dev.asset_package, &import_result);
    
    cx_gfx_mesh_create(&g_dev.gizmos.control_s_x.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[3]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_s_y.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[4]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_s_z.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[2]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_s_xy.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[1]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_s_xz.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[5]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_s_yz.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[6]->asset_.p_data_))->p_primitives[0]);
    cx_gfx_mesh_create(&g_dev.gizmos.control_s_center.gfx_mesh, &((struct static_mesh*)(import_result.p_meshes[0]->asset_.p_data_))->p_primitives[0]);
    
    gltf_free(&gltf);
    import_gltf_free(&import_result);

    g_dev.gizmos.control_t_x.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_T_X;
    g_dev.gizmos.control_t_y.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_T_Y;
    g_dev.gizmos.control_t_z.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_T_Z;
    g_dev.gizmos.control_t_xy.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_T_XY;
    g_dev.gizmos.control_t_xz.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_T_XZ;
    g_dev.gizmos.control_t_yz.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_T_YZ;
    g_dev.gizmos.control_t_center.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_T_CENTER;
    g_dev.gizmos.control_r_x.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_R_X;
    g_dev.gizmos.control_r_y.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_R_Y;
    g_dev.gizmos.control_r_z.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_R_Z;
    g_dev.gizmos.control_r_center.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_R_CENTER;
    g_dev.gizmos.control_s_x.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_S_X;
    g_dev.gizmos.control_s_y.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_S_Y;
    g_dev.gizmos.control_s_z.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_S_Z;
    g_dev.gizmos.control_s_xy.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_S_XY;
    g_dev.gizmos.control_s_xz.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_S_XZ;
    g_dev.gizmos.control_s_yz.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_S_YZ;
    g_dev.gizmos.control_s_center.mesh_id_capturer_id = DEV_MESH_ID_CAPTURER_ID_GIZMO_S_CENTER;


	g_dev.gizmos.control_t_x.color_ka[3] =
	g_dev.gizmos.control_t_y.color_ka[3] =
	g_dev.gizmos.control_t_z.color_ka[3] =
	g_dev.gizmos.control_t_center.color_ka[3] =
	g_dev.gizmos.hovered_control_color_ka[3] = 1.0f;

	vec3_set(.961f, .306f, .306f, g_dev.gizmos.control_t_x.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_x.color_ka, g_dev.gizmos.control_r_x.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_x.color_ka, g_dev.gizmos.control_s_x.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_x.color_ka, g_dev.gizmos.control_t_yz.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_x.color_ka, g_dev.gizmos.control_s_yz.color_ka);

	vec3_set(.525f, .941f, .090f, g_dev.gizmos.control_t_y.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_y.color_ka, g_dev.gizmos.control_r_y.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_y.color_ka, g_dev.gizmos.control_s_y.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_y.color_ka, g_dev.gizmos.control_t_xz.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_y.color_ka, g_dev.gizmos.control_s_xz.color_ka);
    
	vec3_set(.243f, .478f,  1.0f, g_dev.gizmos.control_t_z.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_z.color_ka, g_dev.gizmos.control_r_z.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_z.color_ka, g_dev.gizmos.control_s_z.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_z.color_ka, g_dev.gizmos.control_t_xy.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_z.color_ka, g_dev.gizmos.control_s_xy.color_ka);
    
    vec3_set_s(.8f, g_dev.gizmos.control_t_center.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_center.color_ka, g_dev.gizmos.control_r_center.color_ka);
	vec_copy(4, g_dev.gizmos.control_t_center.color_ka, g_dev.gizmos.control_s_center.color_ka);
    
	vec3_set(.9f, .8f, .3f, g_dev.gizmos.hovered_control_color_ka);
	
	struct mesh_primitive mesh_primitive;

	cx_mesh_gen_sphere(0.5f, 8, 16, &mesh_primitive);
	cx_gfx_mesh_create(&g_dev.physics_collider_mesh_sphere, &mesh_primitive);
	cx_mesh_gen_free(&mesh_primitive);

	cx_mesh_gen_cylinder(0.5f, 0.5f, 0.5f, 16, 0, 0, &mesh_primitive);
	cx_gfx_mesh_create(&g_dev.physics_collider_mesh_capsule_mid, &mesh_primitive);
	cx_mesh_gen_free(&mesh_primitive);

	cx_mesh_gen_hemisphere(0.5f, 4, 16, &mesh_primitive);
	cx_gfx_mesh_create(&g_dev.physics_collider_mesh_capsule_cap, &mesh_primitive);
	cx_mesh_gen_free(&mesh_primitive);

	cx_mesh_gen_quad(0.5f, 0.5f, (float[]){ 0, 1, 0 }, &mesh_primitive);
	cx_gfx_mesh_create(&g_dev.physics_collider_mesh_plane, &mesh_primitive);
	cx_mesh_gen_free(&mesh_primitive);

	hashtable_init(&g_dev.physics_hull_meshes, sizeof(struct cx_gfx_mesh));

	cx_commands_register(&(struct cx_command_info){
		.s_id = "physics.tgl_draw_dbg",
		.s_description = "Toggle debug rendering of physics colliders."
	}, cmd_toggle_draw_physics_colliders);
}

void dev_shutdown(void) {
	mesh_id_capturer_free(&g_dev.mesh_id_capturer);

    input_event_unsubscribe(INPUT_EVENT_key, on_key);
    input_event_unsubscribe(INPUT_EVENT_mouse_button, on_mouse_button);
    input_event_unsubscribe(INPUT_EVENT_mouse_move, on_mouse_move);

	cx_gfx_program_destroy(&g_dev.program_flat);
	cx_gfx_program_param_buffer_destroy(&g_dev.program_pbuffer_camera);
	cx_gfx_program_param_buffer_destroy(&g_dev.program_pbuffer_object);
	cx_gfx_program_param_buffer_destroy(&g_dev.program_pbuffer_material);

    cx_gfx_mesh_destroy(&g_dev.gizmos.control_t_x.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_t_y.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_t_z.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_t_xy.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_t_xz.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_t_yz.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_t_center.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_r_x.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_r_y.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_r_z.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_r_center.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_s_x.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_s_y.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_s_z.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_s_xy.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_s_xz.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_s_yz.gfx_mesh);
    cx_gfx_mesh_destroy(&g_dev.gizmos.control_s_center.gfx_mesh);

    asset_package_free(&g_dev.asset_package);
}

void dev_draw(
	const struct cx_gfx_framebuffer* p_framebuffer, 
	uint32_t framebuffer_width, uint32_t framebuffer_height,
	const float* p_projection_matrix,
	const float* p_view_matrix) {
	
	ENSURE_DEV_MODE();

    matrix_multiply(p_projection_matrix, p_view_matrix, g_dev.perspective_view_matrix);
    
    g_dev.perspective_scale = 2 / p_projection_matrix[5];

    float view_matrix_inverse[16];
    matrix_inverse(4, p_view_matrix, view_matrix_inverse);
    vec3_copy(&view_matrix_inverse[12], g_dev.camera_position);

	cx_gfx_framebuffer_bind(p_framebuffer);

	cx_gfx_program_bind(&g_dev.program_flat);

    cx_gfx_program_param_block_bind_buffer(&g_dev.program_pblock_camera, &g_dev.program_pbuffer_camera, 0, 0);
    cx_gfx_program_param_block_bind_buffer(&g_dev.program_pblock_object, &g_dev.program_pbuffer_object, 0, 0);
    cx_gfx_program_param_block_bind_buffer(&g_dev.program_pblock_material, &g_dev.program_pbuffer_material, 0, 0);

	struct {
		float projection_matrix[16];
		float view_matrix[16];
	} camera;

	matrix_copy(p_projection_matrix, camera.projection_matrix);
	matrix_copy(p_view_matrix, camera.view_matrix);

	cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_camera, 0, 0, &camera);

    draw_hull_DEBUGDEBUGDEBUG();

    if (g_dev.b_draw_physics) {
        draw_physics();
    }

    if (g_dev.gizmos.p_target_transform) {
        draw_gizmo();

        if (g_dev.p_selected_entity->p_mesh) {
            float bounds_min[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
            float bounds_max[3] = { FLT_MIN, FLT_MIN, FLT_MIN };

            const struct static_mesh* p_mesh = g_dev.p_selected_entity->p_mesh->asset_.p_data_;

            for (size_t i = 0; i < p_mesh->num_primitives; ++i) {
                const struct cx_gfx_mesh* p_gfx_mesh = &p_mesh->p_gfx_meshes[i];
                vec3_min(bounds_min, p_gfx_mesh->bounds_min_, bounds_min);
                vec3_max(bounds_max, p_gfx_mesh->bounds_max_, bounds_max);
            }

            float mesh_bounds_box_vertices[] = {
                bounds_min[0], bounds_min[1], bounds_min[2],
                bounds_max[0], bounds_min[1], bounds_min[2],
                bounds_min[0], bounds_min[1], bounds_max[2],
                bounds_max[0], bounds_min[1], bounds_max[2],
                bounds_min[0], bounds_min[1], bounds_min[2],
                bounds_min[0], bounds_min[1], bounds_max[2],
                bounds_max[0], bounds_min[1], bounds_min[2],
                bounds_max[0], bounds_min[1], bounds_max[2],
                bounds_min[0], bounds_max[1], bounds_min[2],
                bounds_max[0], bounds_max[1], bounds_min[2],
                bounds_min[0], bounds_max[1], bounds_max[2],
                bounds_max[0], bounds_max[1], bounds_max[2],
                bounds_min[0], bounds_max[1], bounds_min[2],
                bounds_min[0], bounds_max[1], bounds_max[2],
                bounds_max[0], bounds_max[1], bounds_min[2],
                bounds_max[0], bounds_max[1], bounds_max[2],
                bounds_min[0], bounds_min[1], bounds_min[2],
                bounds_min[0], bounds_max[1], bounds_min[2],
                bounds_max[0], bounds_min[1], bounds_min[2],
                bounds_max[0], bounds_max[1], bounds_min[2],
                bounds_min[0], bounds_min[1], bounds_max[2],
                bounds_min[0], bounds_max[1], bounds_max[2],
                bounds_max[0], bounds_min[1], bounds_max[2],
                bounds_max[0], bounds_max[1], bounds_max[2]
            };

            struct vertex_buffer mesh_bounds_box_vertex_buffer = {
                .p_bytes = mesh_bounds_box_vertices,
                .size = sizeof(mesh_bounds_box_vertices)
            };

            struct vertex_attribute mesh_bounds_box_vertex_attribute = {
                .index = 0,
                .vertex_buffer_index = 0,
                .layout = {
                    .component_count = 3,
                    .component_type = VERTEX_ATTRIBUTE_TYPE_f32
                }
            };

            struct mesh_primitive mesh_bounds_box_mesh = {
                .p_vertex_buffers = &mesh_bounds_box_vertex_buffer,
                .num_vertex_buffers = 1,
                .p_attributes = &mesh_bounds_box_vertex_attribute,
                .num_attributes = 1,
                .vertex_count = 24,
                .draw_mode = MESH_PRIMITIVE_DRAW_MODE_lines
            };

            struct cx_gfx_mesh gfx_mesh_bounds_box_mesh;
            cx_gfx_mesh_create(&gfx_mesh_bounds_box_mesh, &mesh_bounds_box_mesh);
            
			cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, g_dev.p_selected_entity->transform.world_trs_matrix);
            
            float mesh_bounds_box_color[] = { 1.f, .65f, 1.f, 1.f };
			cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_material, 0, 0, mesh_bounds_box_color);

            cx_gfx_mesh_draw(&gfx_mesh_bounds_box_mesh);

            cx_gfx_mesh_destroy(&gfx_mesh_bounds_box_mesh);
        }
    }

	{
		mesh_id_capturer_begin(&g_dev.mesh_id_capturer, framebuffer_width, framebuffer_height, p_projection_matrix, p_view_matrix);

		send_scene_to_mesh_id_capturer();
		
		if (g_dev.p_selected_entity) {
			send_gizmo_to_mesh_id_capturer();
		}
		
		int mouse_client_coords[2];
		platform_window_get_mouse_client_coords(g_dev.p_window, &mouse_client_coords[0], &mouse_client_coords[1]);

		float mouse_position_normalized[2];
		platform_window_normalize_client_coords(g_dev.p_window, mouse_client_coords[0], mouse_client_coords[1], &mouse_position_normalized[0], &mouse_position_normalized[1]);

		g_dev.target_mesh_id = mesh_id_capturer_query(&g_dev.mesh_id_capturer, mouse_position_normalized[0], mouse_position_normalized[1]);
	}
}

void on_key(const void* p_event_data, void* p_user_ptr) {
	(void)p_user_ptr;

	ENSURE_DEV_MODE();

    const struct input_event_data_key* p_e = p_event_data;
    
    if (p_e->b_is_down) {
        return;
    }

    switch (p_e->key) {
        case KEY_1: {
			toggle_draw_physics_colliders();
            break;
        }
        
        default: break;
    }

    if (g_dev.gizmos.p_target_transform == 0 || g_dev.pressed_mesh_id) {
        return;
    }

    switch (p_e->key) {
        // Switch between local and global gizmo axes
        case KEY_q: {
            g_dev.gizmos.b_use_local_axes = !g_dev.gizmos.b_use_local_axes;
            break;
        }

        // Enable translation gizmo
        case KEY_e: {
            g_dev.gizmos.active_type = GIZMO_TYPE_translate;
            break;
        }

        // Enable rotation gizmo
        case KEY_r: {
            g_dev.gizmos.active_type = GIZMO_TYPE_rotate;
            break;
        }

        // Enable scale gizmo
        case KEY_t: {
            g_dev.gizmos.active_type = GIZMO_TYPE_scale;
            break;
        }

        // Reset selected entity transform
        case KEY_y: {
            transform_reset_local(g_dev.gizmos.p_target_transform);
            break;
        }

        // Cycle between static physics object, rigidboy physics object, and no physics object
        case KEY_o: {
            if (!g_dev.p_selected_entity->p_physics_object) {
                g_dev.p_selected_entity->p_physics_object = physics_world_new_object(g_dev.p_physics_world, &g_dev.p_selected_entity->transform, 0);
                physics_world_new_object_collider(g_dev.p_physics_world, g_dev.p_selected_entity->p_physics_object, PHYSICS_COLLIDER_TYPE_sphere);
            } else if (!g_dev.p_selected_entity->p_physics_object->b_is_rigidbody_) {
                // todo replace static object with rigidbody with SAME collider
            } else {
                physics_world_destroy_object(g_dev.p_physics_world, g_dev.p_selected_entity->p_physics_object);
                g_dev.p_selected_entity->p_physics_object = 0;
            }
            break;
        }

        // Cycle physics body collider type
        case KEY_i: {
            if (!g_dev.p_selected_entity->p_physics_object) {
                break;
            }

            if (!g_dev.p_selected_entity->p_physics_object->p_collider_) {
                physics_world_new_object_collider(g_dev.p_physics_world, g_dev.p_selected_entity->p_physics_object, PHYSICS_COLLIDER_TYPE_sphere);
            } else if (g_dev.p_selected_entity->p_physics_object->p_collider_->type != PHYSICS_COLLIDER_TYPE_plane) {
                physics_collider_init(g_dev.p_selected_entity->p_physics_object->p_collider_, g_dev.p_selected_entity->p_physics_object->p_collider_->type + 1);

                if (g_dev.p_selected_entity->p_physics_object->p_collider_->type == PHYSICS_COLLIDER_TYPE_hull) {
                    darr_set_length(&g_dev.p_selected_entity->p_physics_object->p_collider_->shape.as_hull.verts, g_dev.num_hull_points);
                    for (size_t i = 0; i < g_dev.num_hull_points; ++i) {
                        float* p_v = darr_get(&g_dev.p_selected_entity->p_physics_object->p_collider_->shape.as_hull.verts, i);
                        vec3_copy(&g_dev.p_hull_points[i * 3], p_v);
                    }
                }
            } else {
                physics_world_destroy_object_collider(g_dev.p_physics_world, g_dev.p_selected_entity->p_physics_object);
            }

            break;
        }

        // Duplicate selected entity and components
        case KEY_g: {
            if (!g_dev.p_selected_entity) {
                break;
            }

            struct scene_entity* p_new_scene_entity = scene_new_entity(g_dev.p_scene);
            
            transform_copy(&g_dev.p_selected_entity->transform, &p_new_scene_entity->transform);
            
            p_new_scene_entity->p_mesh = g_dev.p_selected_entity->p_mesh;

            if (g_dev.p_selected_entity->p_physics_object) {
                p_new_scene_entity->p_physics_object = physics_world_new_object(g_dev.p_selected_entity->p_physics_object->p_world_, 0, g_dev.p_selected_entity->p_physics_object->b_is_rigidbody_);

                if (p_new_scene_entity->p_physics_object->b_is_rigidbody_) {
                    const struct physics_rigidbody* p_rigidbody = (const struct physics_rigidbody*)g_dev.p_selected_entity->p_physics_object;
                    struct physics_rigidbody* p_new_rigidbody = (struct physics_rigidbody*)p_new_scene_entity->p_physics_object;
                    *p_new_rigidbody = *p_rigidbody;
                    p_new_rigidbody->base.p_transform_ = &p_new_scene_entity->transform;
                }

                if (g_dev.p_selected_entity->p_physics_object->p_collider_) {
                    *p_new_scene_entity->p_physics_object->p_collider_ = *g_dev.p_selected_entity->p_physics_object->p_collider_;
                }
            }

            set_selected_entity(p_new_scene_entity);

            CX_LAZYLOG("Scene entity copied\n");

            break;
        }

        case KEY_delete: {
            physics_world_destroy_object(g_dev.p_physics_world, g_dev.p_selected_entity->p_physics_object);
            scene_destroy_entity(g_dev.p_scene, g_dev.p_selected_entity);
            break;
        }

        default: break;
    }
}

void on_mouse_button(const void* p_event_data, void* p_user_ptr) {
	(void)p_user_ptr;

	ENSURE_DEV_MODE();

    const struct input_event_data_mouse_button* p_e = p_event_data;

    if (p_e->button == MOUSE_BUTTON_middle) {
        if (!g_dev.b_is_dragging && !p_e->b_is_down) {
            if (g_dev.target_mesh_id > DEV_MESH_ID_CAPTURER_RESERVED_IDS) {
                size_t entity_id = g_dev.target_mesh_id - DEV_MESH_ID_CAPTURER_RESERVED_IDS;
                struct scene_entity* p_entity = scene_get_entity(g_dev.p_scene, entity_id);

                if (transform_set_local_transform(&g_dev.p_selected_entity->transform, p_entity == g_dev.p_selected_entity ? 0 : &p_entity->transform, 1)) {
                    set_selected_entity(p_entity);
                }
            }
        }
    }

    if (p_e->button == MOUSE_BUTTON_left) {
        if (!g_dev.b_is_dragging && !p_e->b_is_down) {
            if (g_dev.target_mesh_id > DEV_MESH_ID_CAPTURER_RESERVED_IDS) {
                size_t entity_id = g_dev.target_mesh_id - DEV_MESH_ID_CAPTURER_RESERVED_IDS;
                struct scene_entity* p_entity = scene_get_entity(g_dev.p_scene, entity_id);
                set_selected_entity(p_entity == g_dev.p_selected_entity ? 0 : p_entity);
            } else if (g_dev.target_mesh_id == 0) {
                set_selected_entity(0);
            }
        }

        if (!p_e->b_is_down) {
            g_dev.b_is_dragging = 0;
            g_dev.pressed_mesh_id = 0;
        } else {
            g_dev.pressed_mesh_id = g_dev.target_mesh_id;
        };
    }
}

void on_mouse_move(const void* p_event_data, void* p_user_ptr) {
	(void)p_event_data;
	(void)p_user_ptr;

	ENSURE_DEV_MODE();

    if (g_dev.pressed_mesh_id == 0 || g_dev.pressed_mesh_id > DEV_MESH_ID_CAPTURER_RESERVED_IDS) {
        return;
    }

    int mouse_client_coords[2];
    platform_window_get_mouse_client_coords(g_dev.p_window, &mouse_client_coords[0], &mouse_client_coords[1]);

    float cursor_ray[3];
    platform_window_client_to_world_ray(g_dev.p_window, g_dev.perspective_view_matrix, mouse_client_coords[0], mouse_client_coords[1], cursor_ray);
 
    switch (g_dev.pressed_mesh_id) {
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_T_X: {
            float axis[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[0], axis);
            gizmo_drag_translate_axis(axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_T_Y: {
            float axis[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[4], axis);
            gizmo_drag_translate_axis(axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_T_Z: {
            float axis[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[8], axis);
            gizmo_drag_translate_axis(axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_T_XY: {
            float plane_normal[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[8], plane_normal);
            gizmo_drag_translate_plane(plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_T_XZ: {
            float plane_normal[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[4], plane_normal);
            gizmo_drag_translate_plane(plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_T_YZ: {
            float plane_normal[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[0], plane_normal);
            gizmo_drag_translate_plane(plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_T_CENTER: {
            float plane_normal[3];
            vec3_mul_s(cursor_ray, -1, plane_normal);
            gizmo_drag_translate_plane(plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }

        case DEV_MESH_ID_CAPTURER_ID_GIZMO_R_X: {
            float axis[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[0], axis);
            gizmo_drag_rotate(axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_R_Y: {
            float axis[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[4], axis);
            gizmo_drag_rotate(axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_R_Z: {
            float axis[3];
            vec3_norm(&g_dev.gizmos.gizmo_transform[8], axis);
            gizmo_drag_rotate(axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_R_CENTER: {
            float control_plane_normal[3];
            vec3_mul_s(cursor_ray, -1, control_plane_normal);
            gizmo_drag_rotate_ball(control_plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }

        case DEV_MESH_ID_CAPTURER_ID_GIZMO_S_X: {
            float control_axis[3] = { 1, 0, 0 };
            gizmo_drag_scale_axis(control_axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_S_Y: {
            float control_axis[] = { 0, 1, 0 };
            gizmo_drag_scale_axis(control_axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_S_Z: {
            float control_axis[] = { 0, 0, 1 };
            gizmo_drag_scale_axis(control_axis, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_S_XY: {
            float control_plane_normal[3] = { 0, 0, 1 };
            gizmo_drag_scale_plane(control_plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_S_XZ: {
            float control_plane_normal[3] = { 0, 1, 0 };
            gizmo_drag_scale_plane(control_plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_S_YZ: {
            float control_plane_normal[3] = { 1, 0, 0 };
            gizmo_drag_scale_plane(control_plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }
        
        case DEV_MESH_ID_CAPTURER_ID_GIZMO_S_CENTER: {
            float control_plane_normal[3];
            vec3_mul_s(cursor_ray, -1, control_plane_normal);
            gizmo_drag_scale_uniformly(control_plane_normal, g_dev.camera_position, cursor_ray);
            break;
        }
    }
}

void set_selected_entity(struct scene_entity* p_entity) {
    g_dev.p_selected_entity = p_entity;
    g_dev.gizmos.p_target_transform = p_entity ? &p_entity->transform : 0;
}

void toggle_draw_physics_colliders(void) {
	g_dev.b_draw_physics = !g_dev.b_draw_physics;
	CX_LOG(INFO, DEV, g_dev.b_draw_physics ? "Physics collider drawing enabled\n" : "Physics collider drawing disabled\n");
}

void draw_physics(void) {
    for (size_t i = 0; i < g_dev.p_scene->entities_.length_; ++i) {
        struct scene_entity* p_entity = *(struct scene_entity**)darr_get(&g_dev.p_scene->entities_, i);

        if (!p_entity->p_physics_object || !p_entity->p_physics_object->p_collider_) {
            continue;
        }

		float color_ka[] = { 1.0f, 0.85f, 0.4f, 0.3f };
		cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_material, 0, 0, color_ka);

		switch (p_entity->p_physics_object->p_collider_->type) {
			case PHYSICS_COLLIDER_TYPE_sphere: {
				float p_trs[16];

				compute_physics_collider_mesh_trs_sphere(p_entity, p_trs);

				cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, p_trs);
				cx_gfx_mesh_draw(&g_dev.physics_collider_mesh_sphere);
				
				break;
			}

			case PHYSICS_COLLIDER_TYPE_capsule: {
				float p_trs_mid[16];
				float p_trs_cap_p0[16];
				float p_trs_cap_p1[16];

				compute_physics_collider_mesh_trs_capsule(p_entity, p_trs_mid, p_trs_cap_p0, p_trs_cap_p1);

				cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, p_trs_mid);
				cx_gfx_mesh_draw(&g_dev.physics_collider_mesh_capsule_mid);
				
				cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, p_trs_cap_p0);
				cx_gfx_mesh_draw(&g_dev.physics_collider_mesh_capsule_cap);

				cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, p_trs_cap_p1);
				cx_gfx_mesh_draw(&g_dev.physics_collider_mesh_capsule_cap);
				break;
			}

			case PHYSICS_COLLIDER_TYPE_hull: {
				const struct physics_hull* p_hull = &p_entity->p_physics_object->p_collider_->shape.as_hull;

				struct cx_gfx_mesh* p_hull_mesh;

				struct hashtable_itr itr;
				if (!hashtable_find(&g_dev.physics_hull_meshes, &p_entity->id_, sizeof(p_entity->id_), &itr)) {
					p_hull_mesh = hashtable_add(&g_dev.physics_hull_meshes, &p_entity->id_, sizeof(p_entity->id_));
					
					struct he_mesh he_mesh;
					quickhull(p_hull->verts.p_buffer_, p_hull->verts.length_, &he_mesh);

					struct mesh_primitive mesh_primitive;
					cx_mesh_gen_from_halfedge_mesh(&he_mesh, &mesh_primitive);

					cx_gfx_mesh_create(p_hull_mesh, &mesh_primitive);

					quickhull_free(&he_mesh);
					cx_mesh_gen_free(&mesh_primitive);
				} else {
					p_hull_mesh = itr.p_value;
				}

				cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, p_entity->transform.world_trs_matrix);
				cx_gfx_mesh_draw(p_hull_mesh);

				break;
			}

			case PHYSICS_COLLIDER_TYPE_plane: {
				float p_trs[16];

				compute_physics_collider_mesh_trs_plane(p_entity, p_trs);

				cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, p_trs);
				cx_gfx_mesh_draw(&g_dev.physics_collider_mesh_plane);
				break;
			}

			default: break;
		}
    }
}

void compute_physics_collider_mesh_trs_sphere(const struct scene_entity* p_entity, float* p_out_trs) {
	physics_collider_apply_transform(p_entity->p_physics_object->p_collider_, &p_entity->transform);
	
	const struct physics_sphere* p_sphere = &p_entity->p_physics_object->p_collider_->shape.as_sphere;
	
	float s[3];
	float r[4];

	vec3_set_s(p_sphere->radius * 2, s);

	quaternion_identity(r);

	matrix_make_trs(p_sphere->center, r, s, p_out_trs);
	
	physics_collider_undo_transform(p_entity->p_physics_object->p_collider_);
}

void compute_physics_collider_mesh_trs_capsule(
	const struct scene_entity* p_entity,
	float* p_out_trs_mid, float* p_out_trs_cap_p0, float* p_out_trs_cap_p1) {

	physics_collider_apply_transform(p_entity->p_physics_object->p_collider_, &p_entity->transform);
	
	const struct physics_capsule* p_capsule = &p_entity->p_physics_object->p_collider_->shape.as_capsule;

	float s[3];
	float r[4];
	float t[3];

	// Capsule mid section
	
	float p0_p1[3];

	vec3_sub(p_capsule->p1, p_capsule->p0, p0_p1);

	vec3_add(p_capsule->p0, p_capsule->p1, t);
	vec3_mul_s(t, 0.5f, t);

	quaternion_find_rotation_between((const float[]){ 0, 1, 0 }, p0_p1, r);

	s[0] = s[2] = p_capsule->radius * 2;
	s[1] = vec3_len(p0_p1);
	
	matrix_make_trs(t, r, s, p_out_trs_mid);
	
	// Capsule p1 cap

	vec3_copy(p_capsule->p1, t);
	s[1] = s[0];
	matrix_make_trs(t, r, s, p_out_trs_cap_p1);

	// Capsule p0 cap

	float q[4];

	vec3_copy(p_capsule->p0, t);
	quaternion_from_axis_angle((const float[]){ 1, 0, 0 }, M_PI, q);
	quaternion_multiply(r, q, r);

	matrix_make_trs(t, r, s, p_out_trs_cap_p0);

	physics_collider_undo_transform(p_entity->p_physics_object->p_collider_);
}

void compute_physics_collider_mesh_trs_plane(const struct scene_entity* p_entity, float* p_out_trs) {
	const float s = 10.0f;
	matrix_make_trs(
		p_entity->transform.world_position,
		p_entity->transform.world_rotation,
		(const float[]){ s, s, s }, p_out_trs);
}

void draw_gizmo(void) {
    glClear(GL_DEPTH_BUFFER_BIT);

    const float gizmo_viewport_size = 0.15f;
    
    float object_to_camera[3];
    vec3_sub(g_dev.camera_position, g_dev.gizmos.p_target_transform->world_position, object_to_camera);
    float scale = g_dev.perspective_scale * vec3_len(object_to_camera) * gizmo_viewport_size;
    
    matrix_make_uniform_scale(scale, g_dev.gizmos.gizmo_transform);
    
    if (g_dev.gizmos.b_use_local_axes || g_dev.gizmos.active_type == GIZMO_TYPE_scale) {
        float local_rotation[16];
        matrix_make_rotation_from_quaternion(g_dev.gizmos.p_target_transform->world_rotation, local_rotation);
        matrix_multiply(local_rotation, g_dev.gizmos.gizmo_transform, g_dev.gizmos.gizmo_transform);
    }

    float translation[16];
    matrix_make_translation(g_dev.gizmos.p_target_transform->world_position[0], g_dev.gizmos.p_target_transform->world_position[1], g_dev.gizmos.p_target_transform->world_position[2], translation);
    matrix_multiply(translation, g_dev.gizmos.gizmo_transform, g_dev.gizmos.gizmo_transform);
    
	cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, g_dev.gizmos.gizmo_transform);

    switch (g_dev.gizmos.active_type) {
        case GIZMO_TYPE_translate: {
            draw_gizmo_control(&g_dev.gizmos.control_t_x);
            draw_gizmo_control(&g_dev.gizmos.control_t_y);
            draw_gizmo_control(&g_dev.gizmos.control_t_z);
            draw_gizmo_control(&g_dev.gizmos.control_t_xy);
            draw_gizmo_control(&g_dev.gizmos.control_t_xz);
            draw_gizmo_control(&g_dev.gizmos.control_t_yz);
            draw_gizmo_control(&g_dev.gizmos.control_t_center);
            break;
        }
        
        case GIZMO_TYPE_rotate: {
            draw_gizmo_control(&g_dev.gizmos.control_r_x);
            draw_gizmo_control(&g_dev.gizmos.control_r_y);
            draw_gizmo_control(&g_dev.gizmos.control_r_z);
            draw_gizmo_control(&g_dev.gizmos.control_r_center);
            break;
        }
        
        case GIZMO_TYPE_scale: {
            draw_gizmo_control(&g_dev.gizmos.control_s_x);
            draw_gizmo_control(&g_dev.gizmos.control_s_y);
            draw_gizmo_control(&g_dev.gizmos.control_s_z);
            draw_gizmo_control(&g_dev.gizmos.control_s_xy);
            draw_gizmo_control(&g_dev.gizmos.control_s_xz);
            draw_gizmo_control(&g_dev.gizmos.control_s_yz);
            draw_gizmo_control(&g_dev.gizmos.control_s_center);
            break;
        }
    }
}

void draw_gizmo_control(const struct gizmo_control* p_control) {
    const int b_highlight =
		(g_dev.pressed_mesh_id == 0 && g_dev.target_mesh_id == p_control->mesh_id_capturer_id) ||
		(g_dev.pressed_mesh_id == p_control->mesh_id_capturer_id);
	
	cx_gfx_program_param_buffer_set(
		&g_dev.program_pbuffer_material, 0, 0,
		b_highlight ? g_dev.gizmos.hovered_control_color_ka : p_control->color_ka);
    
	cx_gfx_mesh_draw(&p_control->gfx_mesh);
}

int find_gizmo_control_plane_cursor_drag_intersection(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_pos) {
    float control_plane_d = -vec3_dot(p_control_plane_normal, &g_dev.gizmos.gizmo_transform[12]);

    if (!ray_plane_intersection(p_cursor_ray_origin, p_cursor_ray, p_control_plane_normal, control_plane_d, p_cursor_world_pos)) {
        return 0;
    }

    if (g_dev.b_is_dragging) {
        return 1;
    }
    
    vec3_copy(p_cursor_world_pos, g_dev.gizmos.cursor_drag_last_world_pos);
    g_dev.b_is_dragging = 1;
    return 0;
}

void compute_gizmo_control_axis_drag_plane_normal(const float* p_control_axis, const float* p_view_pos, float* p_plane_norm_d) {
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

void compute_gizmo_control_plane_cursor_drag_delta(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_delta) {
    float cursor_ray_intersection[3];
    if (!find_gizmo_control_plane_cursor_drag_intersection(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, cursor_ray_intersection)) {
        vec3_clr(p_cursor_world_delta);
        return;
    }

    vec3_sub(cursor_ray_intersection, g_dev.gizmos.cursor_drag_last_world_pos, p_cursor_world_delta);
    
    vec3_copy(cursor_ray_intersection, g_dev.gizmos.cursor_drag_last_world_pos);
}

float compute_gizmo_control_plane_cursor_drag_delta_angle(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float p0[3];
    float p1[3];

    if (!find_gizmo_control_plane_cursor_drag_intersection(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, p1)) {
        return 0;
    }

    vec3_sub(g_dev.gizmos.cursor_drag_last_world_pos, &g_dev.gizmos.gizmo_transform[12], p0);
    vec3_copy(p1, g_dev.gizmos.cursor_drag_last_world_pos);

    vec3_sub(p1, &g_dev.gizmos.gizmo_transform[12], p1);
    
    float cross[3];
    vec3_cross(p0, p1, cross);
    
    return atan2(vec3_dot(p_control_plane_normal, cross), vec3_dot(p0, p1));
}

void compute_gizmo_control_axis_cursor_drag_delta(const float* p_control_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_delta) {
    float n[3];
    vec3_norm(p_control_axis, n);

    float plane_norm[3];
    compute_gizmo_control_axis_drag_plane_normal(n, p_cursor_ray_origin, plane_norm);

    compute_gizmo_control_plane_cursor_drag_delta(plane_norm, p_cursor_ray_origin, p_cursor_ray, p_cursor_world_delta);

    float dot = vec3_dot(p_cursor_world_delta, n);

    vec3_mul_s(n, dot, p_cursor_world_delta);
}

void gizmo_drag_translate_axis(const float* p_control_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float cursor_world_delta[3];
    compute_gizmo_control_axis_cursor_drag_delta(p_control_axis, p_cursor_ray_origin, p_cursor_ray, cursor_world_delta);

    vec3_add(g_dev.gizmos.p_target_transform->position, cursor_world_delta, g_dev.gizmos.p_target_transform->position);
}

void gizmo_drag_translate_plane(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float cursor_world_delta[3];
    compute_gizmo_control_plane_cursor_drag_delta(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, cursor_world_delta);
    
    vec3_add(g_dev.gizmos.p_target_transform->position, cursor_world_delta, g_dev.gizmos.p_target_transform->position);
}

void gizmo_drag_rotate(const float* p_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float angle = compute_gizmo_control_plane_cursor_drag_delta_angle(p_axis, p_cursor_ray_origin, p_cursor_ray);
    if (FLT_CMP(angle, 0)) {
        return;
    }

    float rotation[4];
    quaternion_from_axis_angle(p_axis, angle, rotation);
    quaternion_multiply(rotation, g_dev.gizmos.p_target_transform->rotation, g_dev.gizmos.p_target_transform->rotation);
    vec_norm(4, g_dev.gizmos.p_target_transform->rotation, g_dev.gizmos.p_target_transform->rotation);
}

void gizmo_drag_rotate_ball(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float cursor_world_delta[3];
    compute_gizmo_control_plane_cursor_drag_delta(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, cursor_world_delta);

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
    vec_norm(4, rotation, rotation);
    
    quaternion_multiply(rotation, g_dev.gizmos.p_target_transform->rotation, g_dev.gizmos.p_target_transform->rotation);
    vec_norm(4, g_dev.gizmos.p_target_transform->rotation, g_dev.gizmos.p_target_transform->rotation);
}

void gizmo_drag_scale_axis(const float* p_control_axis, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float cursor_world_delta[3];
    compute_gizmo_control_axis_cursor_drag_delta(p_control_axis, p_cursor_ray_origin, p_cursor_ray, cursor_world_delta);
    
    const float mod = 0.3f;
    vec3_mul_s(cursor_world_delta, mod, cursor_world_delta);
    
    float new_world_scale[3];
    vec3_add(g_dev.gizmos.p_target_transform->world_scale, cursor_world_delta, new_world_scale);
    transform_set_world_scale(g_dev.gizmos.p_target_transform, new_world_scale);
}

void gizmo_drag_scale_plane(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float cursor_world_delta[3];
    compute_gizmo_control_plane_cursor_drag_delta(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, cursor_world_delta);

    const float mod = 0.3f;
    vec3_mul_s(cursor_world_delta, mod, cursor_world_delta);

    float new_world_scale[3];
    vec3_add(g_dev.gizmos.p_target_transform->world_scale, cursor_world_delta, new_world_scale);
    transform_set_world_scale(g_dev.gizmos.p_target_transform, new_world_scale);
}

void gizmo_drag_scale_uniformly(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float cursor_world_delta[3];
    compute_gizmo_control_plane_cursor_drag_delta(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, cursor_world_delta);

    const float avg = (cursor_world_delta[0] + cursor_world_delta[1] + cursor_world_delta[2]) / 3;
    const float mod = 0.3f;

    float new_world_scale[3];
    vec3_add_s(g_dev.gizmos.p_target_transform->world_scale, avg * mod, new_world_scale);
    transform_set_world_scale(g_dev.gizmos.p_target_transform, new_world_scale);
}

void send_scene_to_mesh_id_capturer(void) {
    for (size_t i = 0; i < g_dev.p_scene->entities_.length_; ++i) {
        struct scene_entity* p_entity = *(struct scene_entity**)darr_get(&g_dev.p_scene->entities_, i);

        const unsigned int mesh_id_capturer_entity_id = DEV_MESH_ID_CAPTURER_RESERVED_IDS + p_entity->id_;

        if (p_entity->p_mesh) {
            const struct static_mesh* p_mesh = p_entity->p_mesh->asset_.p_data_;

            for (size_t j = 0; j < p_mesh->num_primitives; ++j) {
                const struct cx_gfx_mesh* p_gfx_mesh = &p_mesh->p_gfx_meshes[j];

                mesh_id_capturer_draw_item(&(struct mesh_id_capturer_item) {
					.p_mesh = p_gfx_mesh,
					.p_transform = p_entity->transform.world_trs_matrix,
					.id = mesh_id_capturer_entity_id
				});
            }
        }

		if (!g_dev.b_draw_physics || !p_entity->p_physics_object || !p_entity->p_physics_object->p_collider_) {
			continue;
		}

		float color[] = { 1.0f, 1.0f, 1.0f };
		cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_material, 0, 0, color);

		struct mesh_id_capturer_item item = {
			.p_transform = p_entity->transform.world_trs_matrix,
			.id = mesh_id_capturer_entity_id
		};

		switch (p_entity->p_physics_object->p_collider_->type) {
			case PHYSICS_COLLIDER_TYPE_sphere: {
				float p_trs[16];

				compute_physics_collider_mesh_trs_sphere(p_entity, p_trs);
				
				item.p_transform = p_trs;
				item.p_mesh = &g_dev.physics_collider_mesh_sphere;
				mesh_id_capturer_draw_item(&item);
				
				break;
			}

			case PHYSICS_COLLIDER_TYPE_capsule: {
				float p_trs_mid[16];
				float p_trs_cap_p0[16];
				float p_trs_cap_p1[16];

				compute_physics_collider_mesh_trs_capsule(p_entity, p_trs_mid, p_trs_cap_p0, p_trs_cap_p1);
				
				item.p_transform = p_trs_mid;
				item.p_mesh = &g_dev.physics_collider_mesh_capsule_mid;
				mesh_id_capturer_draw_item(&item);
				
				item.p_transform = p_trs_cap_p0;
				item.p_mesh = &g_dev.physics_collider_mesh_capsule_cap;
				mesh_id_capturer_draw_item(&item);
				
				item.p_transform = p_trs_cap_p1;
				item.p_mesh = &g_dev.physics_collider_mesh_capsule_cap;
				mesh_id_capturer_draw_item(&item);

				break;
			}

			case PHYSICS_COLLIDER_TYPE_hull: {
				struct hashtable_itr itr;
				if (!hashtable_find(&g_dev.physics_hull_meshes, &p_entity->id_, sizeof(p_entity->id_), &itr)) {
					break;
				}

				item.p_mesh = itr.p_value;
				mesh_id_capturer_draw_item(&item);
				break;
			}

			case PHYSICS_COLLIDER_TYPE_plane: {
				float p_trs[16];

				compute_physics_collider_mesh_trs_plane(p_entity, p_trs);

				item.p_transform = p_trs;
				item.p_mesh = &g_dev.physics_collider_mesh_plane;
				mesh_id_capturer_draw_item(&item);
				break;
			}

			default: break;
		}
    }
}

void send_gizmo_to_mesh_id_capturer(void) {
	glClear(GL_DEPTH_BUFFER_BIT);

    switch (g_dev.gizmos.active_type) {
        case GIZMO_TYPE_translate: {
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_t_x);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_t_y);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_t_z);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_t_xy);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_t_xz);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_t_yz);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_t_center);
            break;
        }
        
        case GIZMO_TYPE_rotate: {
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_r_x);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_r_y);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_r_z);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_r_center);
            break;
        }
        
        case GIZMO_TYPE_scale: {
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_s_x);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_s_y);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_s_z);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_s_xy);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_s_xz);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_s_yz);
            send_gizmo_control_to_mesh_id_capturer(&g_dev.gizmos.control_s_center);
            break;
        }
    }
}

void send_gizmo_control_to_mesh_id_capturer(const struct gizmo_control* p_control) {
	mesh_id_capturer_draw_item(&(struct mesh_id_capturer_item) {
		.p_mesh = &p_control->gfx_mesh,
		.p_transform = g_dev.gizmos.gizmo_transform,
		.id = p_control->mesh_id_capturer_id
	});
}

void draw_hull_DEBUGDEBUGDEBUG(void) {
    if (!g_dev.p_selected_entity || !g_dev.p_selected_entity->p_mesh) {
        return;
    }

    static struct he_mesh he_mesh;
    
    static struct mesh_primitive mesh_primitive;
    static struct mesh_primitive mesh_primitive_outline;
    
    static struct cx_gfx_mesh gfx_mesh;
    static struct cx_gfx_mesh gfx_mesh_outline;

    static struct scene_entity* p_old_selected_entity;

    if (g_dev.p_selected_entity != p_old_selected_entity) {
        p_old_selected_entity = g_dev.p_selected_entity;

        if (he_mesh.p_buffer) {
            quickhull_free(&he_mesh);
            cx_mesh_gen_free(&mesh_primitive);
            cx_gfx_mesh_destroy(&gfx_mesh);
            cx_gfx_mesh_destroy(&gfx_mesh_outline);
        }

        quickhull_static_mesh((const struct static_mesh*)g_dev.p_selected_entity->p_mesh->asset_.p_data_, &he_mesh);
        
        cx_mesh_gen_from_halfedge_mesh(&he_mesh, &mesh_primitive);
        cx_gfx_mesh_create(&gfx_mesh, &mesh_primitive);
    
        cx_mesh_gen_from_halfedge_mesh(&he_mesh, &mesh_primitive_outline);
        cx_gfx_mesh_create(&gfx_mesh_outline, &mesh_primitive_outline);
    }

    float matrix[16];
    matrix_make_identity(matrix);
	cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_object, 0, 0, matrix);
    
    float color[] = { 1, 1, 1 };
	cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_material, 0, 0, color);
    cx_gfx_mesh_draw(&gfx_mesh);
    
    float color_wf[] = { 1, 0, 0 };
	cx_gfx_program_param_buffer_set(&g_dev.program_pbuffer_material, 0, 0, color_wf);
    cx_gfx_mesh_draw(&gfx_mesh_outline);
}

void cmd_toggle_draw_physics_colliders(const struct cx_command_context* p_context){
	(void)p_context;
	toggle_draw_physics_colliders();
}
