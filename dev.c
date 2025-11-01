#include <math.h>

#include "asset.h"
#include "dev.h"
#include "gl_mesh.h"
#include "gl_program.h"
#include "gltf.h"
#include "half_edge.h"
#include "import_gltf.h"
#include "input.h"
#include "logging.h"
#include "math_utils.h"
#include "matrix.h"
#include "mesh_factory.h"
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
    struct gl_mesh gl_mesh;
    unsigned int   mesh_id_capturer_id;
    float          color[3];
};

static struct dev_state {
    const struct platform_window* p_platform_window;
    struct scene*                 p_scene;
    struct scene_entity*          p_selected_entity;
    struct mesh_id_capturer       mesh_id_capturer;
    unsigned int                  target_mesh_id;
    unsigned int                  pressed_mesh_id;

    int                           b_is_dragging;

    struct gl_program             gl_program_flat;
    GLuint                        gl_program_flat_u_projection_matrix;
    GLuint                        gl_program_flat_u_view_matrix;
    GLuint                        gl_program_flat_u_model_matrix;
    GLuint                        gl_program_flat_u_color;
    
    struct asset_package          asset_package;

    float                         perspective_view_matrix[16];
    float                         perspective_scale;
    float                         camera_position[3];

    struct physics_world*         p_physics_world;
    struct gl_mesh                gl_physics_collider_meshes[4];
    int                           b_draw_physics;
    float*                        p_hull_points;
    size_t                        num_hull_points;

    struct gizmos_state {
        enum gizmo_type      active_type;
        int                  b_use_local_axes;
        float                gizmo_transform[16];
        struct transform*    p_target_transform;
        float                cursor_drag_last_world_pos[3];
        float                hovered_control_color[3];
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

static void draw_physics(void);

static void compute_physics_collider_transform_matrix(const struct physics_collider* p_collider, const struct transform* p_transform, float* p_collider_transform_matrix);

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

static void mesh_selector_render_pass(size_t framebuffer_width, size_t framebuffer_height, const float* p_projection_matrix, const float* p_view_matrix);
static void mesh_selector_render_pass_submit_gizmo_control(const struct gizmo_control* p_control);

static void draw_hull_DEBUGDEBUGDEBUG(void);

void dev_init(const struct platform_window* p_platform_window, struct scene* p_scene, struct physics_world* p_physics_world) {
    g_dev.p_platform_window = p_platform_window;

    g_dev.p_scene = p_scene;

    g_dev.p_physics_world = p_physics_world;

    mesh_id_capturer_init(&g_dev.mesh_id_capturer);

    input_event_subscribe(INPUT_EVENT_key, on_key, 0);
    input_event_subscribe(INPUT_EVENT_mouse_button, on_mouse_button, 0);
    input_event_subscribe(INPUT_EVENT_mouse_move, on_mouse_move, 0);

    struct gl_shader gl_vertex_shader;
    struct gl_shader gl_fragment_shader;

    gl_shader_create(&gl_vertex_shader, GL_VERTEX_SHADER);
    gl_shader_compile(&gl_vertex_shader,
        "#version 330 core\n"
        "uniform mat4 u_projection_matrix;"
        "uniform mat4 u_view_matrix;"
        "uniform mat4 u_model_matrix;"
        "layout (location=0) in vec3 a_pos;"
        "void main() {"
            "gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(a_pos, 1.0);"
        "}");

    gl_shader_create(&gl_fragment_shader, GL_FRAGMENT_SHADER);
    gl_shader_compile(&gl_fragment_shader,
        "#version 330 core\n"
        "uniform vec3 u_color;"
        "out vec4 f_color;"
        "void main() {"
            "f_color = vec4(u_color, 1.0);"
        "}");

    gl_program_create(&g_dev.gl_program_flat);
    gl_program_attach_shader(&g_dev.gl_program_flat, &gl_vertex_shader);
    gl_program_attach_shader(&g_dev.gl_program_flat, &gl_fragment_shader);
    gl_program_link(&g_dev.gl_program_flat);

    gl_shader_destroy(&gl_vertex_shader);
    gl_shader_destroy(&gl_fragment_shader);

    g_dev.gl_program_flat_u_projection_matrix = glGetUniformLocation(g_dev.gl_program_flat.gl_handle, "u_projection_matrix");
    g_dev.gl_program_flat_u_view_matrix = glGetUniformLocation(g_dev.gl_program_flat.gl_handle, "u_view_matrix");
    g_dev.gl_program_flat_u_model_matrix = glGetUniformLocation(g_dev.gl_program_flat.gl_handle, "u_model_matrix");
    g_dev.gl_program_flat_u_color = glGetUniformLocation(g_dev.gl_program_flat.gl_handle, "u_color");

    asset_package_init(&g_dev.asset_package);

    struct mesh_primitive mesh_primitive;
    
    // Sphere
    mesh_factory_make_uv_sphere_primitive(0.5, 12, &mesh_primitive);
    gl_mesh_create(&g_dev.gl_physics_collider_meshes[PHYSICS_COLLIDER_TYPE_sphere], &mesh_primitive);
    mesh_factory_free_primitive(&mesh_primitive);
    
    // Capsule: todo
    mesh_factory_make_uv_sphere_primitive(0.5, 12, &mesh_primitive);
    gl_mesh_create(&g_dev.gl_physics_collider_meshes[PHYSICS_COLLIDER_TYPE_capsule], &mesh_primitive);
    mesh_factory_free_primitive(&mesh_primitive);

    // Hull
    mesh_factory_make_box(1, 1, 1, &mesh_primitive);
    gl_mesh_create(&g_dev.gl_physics_collider_meshes[PHYSICS_COLLIDER_TYPE_hull], &mesh_primitive);

    struct static_mesh static_mesh = {
        .p_primitives = &mesh_primitive,
        .num_primitives = 1
    };
    struct he_mesh static_mesh_hull;

    quickhull_static_mesh(&static_mesh, &static_mesh_hull);

    half_edge_get_vertices(&static_mesh_hull, 0, &g_dev.num_hull_points);

    g_dev.p_hull_points = malloc(sizeof(float) * 3 * g_dev.num_hull_points);
    half_edge_get_vertices(&static_mesh_hull, g_dev.p_hull_points, &g_dev.num_hull_points);

    quickhull_free(&static_mesh_hull);

    mesh_factory_free_primitive(&mesh_primitive);
    
    // Plane
    mesh_factory_make_plane(10, 10, &mesh_primitive);
    gl_mesh_create(&g_dev.gl_physics_collider_meshes[PHYSICS_COLLIDER_TYPE_plane], &mesh_primitive);
    mesh_factory_free_primitive(&mesh_primitive);

    // load gizmos
    
    g_dev.gizmos.active_type = GIZMO_TYPE_translate;

    struct gltf gltf;
    struct import_gltf_result import_result;

    gltf_load_from_file("gizmo_translate.glb", &gltf);
    import_gltf(&gltf, &g_dev.asset_package, &import_result);
    
    gl_mesh_create(&g_dev.gizmos.control_t_x.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[5]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_t_y.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[6]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_t_z.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[4]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_t_xy.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[2]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_t_xz.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[3]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_t_yz.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[0]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_t_center.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[1]->_asset._p_data))->p_primitives[0]);
    
    gltf_free(&gltf);
    import_gltf_free(&import_result);

    gltf_load_from_file("gizmo_rotate.glb", &gltf);
    import_gltf(&gltf, &g_dev.asset_package, &import_result);
    
    gl_mesh_create(&g_dev.gizmos.control_r_x.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[1]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_r_y.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[2]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_r_z.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[0]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_r_center.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[3]->_asset._p_data))->p_primitives[0]);
    
    gltf_free(&gltf);
    import_gltf_free(&import_result);

    gltf_load_from_file("gizmo_scale.glb", &gltf);
    import_gltf(&gltf, &g_dev.asset_package, &import_result);
    
    gl_mesh_create(&g_dev.gizmos.control_s_x.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[3]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_s_y.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[4]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_s_z.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[2]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_s_xy.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[1]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_s_xz.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[5]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_s_yz.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[6]->_asset._p_data))->p_primitives[0]);
    gl_mesh_create(&g_dev.gizmos.control_s_center.gl_mesh, &((struct static_mesh*)(import_result.p_meshes[0]->_asset._p_data))->p_primitives[0]);
    
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

    g_dev.gizmos.control_t_x.color[0] = g_dev.gizmos.control_r_x.color[0] = g_dev.gizmos.control_s_x.color[0] = g_dev.gizmos.control_t_yz.color[0] = g_dev.gizmos.control_s_yz.color[0] = 0.961;
    g_dev.gizmos.control_t_x.color[1] = g_dev.gizmos.control_r_x.color[1] = g_dev.gizmos.control_s_x.color[1] = g_dev.gizmos.control_t_yz.color[1] = g_dev.gizmos.control_s_yz.color[1] = 0.306f;
    g_dev.gizmos.control_t_x.color[2] = g_dev.gizmos.control_r_x.color[2] = g_dev.gizmos.control_s_x.color[2] = g_dev.gizmos.control_t_yz.color[2] = g_dev.gizmos.control_s_yz.color[2] = 0.306f;
    
    g_dev.gizmos.control_t_y.color[0] = g_dev.gizmos.control_r_y.color[0] = g_dev.gizmos.control_s_y.color[0] = g_dev.gizmos.control_t_xz.color[0] = g_dev.gizmos.control_s_xz.color[0] = 0.525f;
    g_dev.gizmos.control_t_y.color[1] = g_dev.gizmos.control_r_y.color[1] = g_dev.gizmos.control_s_y.color[1] = g_dev.gizmos.control_t_xz.color[1] = g_dev.gizmos.control_s_xz.color[1] = 0.941;
    g_dev.gizmos.control_t_y.color[2] = g_dev.gizmos.control_r_y.color[2] = g_dev.gizmos.control_s_y.color[2] = g_dev.gizmos.control_t_xz.color[2] = g_dev.gizmos.control_s_xz.color[2] = 0.09f;
    
    g_dev.gizmos.control_t_z.color[0] = g_dev.gizmos.control_r_z.color[0] = g_dev.gizmos.control_s_z.color[0] = g_dev.gizmos.control_t_xy.color[0] = g_dev.gizmos.control_s_xy.color[0] = 0.243f;
    g_dev.gizmos.control_t_z.color[1] = g_dev.gizmos.control_r_z.color[1] = g_dev.gizmos.control_s_z.color[1] = g_dev.gizmos.control_t_xy.color[1] = g_dev.gizmos.control_s_xy.color[1] = 0.478f;
    g_dev.gizmos.control_t_z.color[2] = g_dev.gizmos.control_r_z.color[2] = g_dev.gizmos.control_s_z.color[2] = g_dev.gizmos.control_t_xy.color[2] = g_dev.gizmos.control_s_xy.color[2] = 1;
    
    g_dev.gizmos.control_t_center.color[0] = g_dev.gizmos.control_r_center.color[0] = g_dev.gizmos.control_s_center.color[0] = 0.8;
    g_dev.gizmos.control_t_center.color[1] = g_dev.gizmos.control_r_center.color[1] = g_dev.gizmos.control_s_center.color[1] = 0.8;
    g_dev.gizmos.control_t_center.color[2] = g_dev.gizmos.control_r_center.color[2] = g_dev.gizmos.control_s_center.color[2] = 0.8;
    
    g_dev.gizmos.hovered_control_color[0] = 0.9f;
    g_dev.gizmos.hovered_control_color[1] = 0.8f;
    g_dev.gizmos.hovered_control_color[2] = 0.3f;
}

void dev_shutdown(void) {
    mesh_id_capturer_destroy(&g_dev.mesh_id_capturer);

    input_event_unsubscribe(INPUT_EVENT_key, on_key);
    input_event_unsubscribe(INPUT_EVENT_mouse_button, on_mouse_button);
    input_event_unsubscribe(INPUT_EVENT_mouse_move, on_mouse_move);

    gl_program_destroy(&g_dev.gl_program_flat);

    gl_mesh_destroy(&g_dev.gizmos.control_t_x.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_t_y.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_t_z.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_t_xy.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_t_xz.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_t_yz.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_t_center.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_r_x.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_r_y.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_r_z.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_r_center.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_s_x.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_s_y.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_s_z.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_s_xy.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_s_xz.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_s_yz.gl_mesh);
    gl_mesh_destroy(&g_dev.gizmos.control_s_center.gl_mesh);

    asset_package_free(&g_dev.asset_package);
}

void dev_draw(GLuint gl_framebuffer, size_t framebuffer_width, size_t framebuffer_height, const float* p_projection_matrix, const float* p_view_matrix) {
    matrix_multiply(p_projection_matrix, p_view_matrix, g_dev.perspective_view_matrix);
    
    g_dev.perspective_scale = 2 / p_projection_matrix[5];

    float view_matrix_inverse[16];
    matrix_inverse(4, p_view_matrix, view_matrix_inverse);
    vec3_set(&view_matrix_inverse[12], g_dev.camera_position);

    glBindFramebuffer(GL_FRAMEBUFFER, gl_framebuffer);

    glUseProgram(g_dev.gl_program_flat.gl_handle);
                
    glUniformMatrix4fv(g_dev.gl_program_flat_u_projection_matrix, 1, GL_FALSE, p_projection_matrix);
    glUniformMatrix4fv(g_dev.gl_program_flat_u_view_matrix, 1, GL_FALSE, p_view_matrix);

    draw_hull_DEBUGDEBUGDEBUG();

    if (g_dev.b_draw_physics) {
        draw_physics();
    }

    if (g_dev.gizmos.p_target_transform) {
        draw_gizmo();

        if (g_dev.p_selected_entity->p_mesh) {
            float bounds_min[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
            float bounds_max[3] = { FLT_MIN, FLT_MIN, FLT_MIN };

            const struct static_mesh* p_mesh = g_dev.p_selected_entity->p_mesh->_asset._p_data;

            for (size_t i = 0; i < p_mesh->num_primitives; ++i) {
                const struct gl_mesh* p_gl_mesh = &p_mesh->p_gl_meshes[i];
                vec3_min(bounds_min, p_gl_mesh->_bounds_min, bounds_min);
                vec3_max(bounds_max, p_gl_mesh->_bounds_max, bounds_max);
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

            struct gl_mesh gl_mesh_bounds_box_mesh;
            gl_mesh_create(&gl_mesh_bounds_box_mesh, &mesh_bounds_box_mesh);
            
            glUniformMatrix4fv(g_dev.gl_program_flat_u_model_matrix, 1, GL_FALSE, g_dev.p_selected_entity->transform.world_trs_matrix);
            
            float mesh_bounds_box_color[] = { 0.75, 0.6, 0 };
            glUniform3fv(g_dev.gl_program_flat_u_color, 1, mesh_bounds_box_color);

            gl_mesh_draw(&gl_mesh_bounds_box_mesh);

            gl_mesh_destroy(&gl_mesh_bounds_box_mesh);
        }
    }

    mesh_selector_render_pass(framebuffer_width, framebuffer_height, p_projection_matrix, p_view_matrix);
}

void on_key(const void* p_event_data, void* p_user_ptr) {
    const struct input_event_data_key* p_e = p_event_data;
    
    if (p_e->b_is_down) {
        return;
    }

    switch (p_e->key) {
        case KEY_1: {
            g_dev.b_draw_physics = !g_dev.b_draw_physics;
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
            } else if (!g_dev.p_selected_entity->p_physics_object->_b_is_rigidbody) {
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

            if (!g_dev.p_selected_entity->p_physics_object->_p_collider) {
                physics_world_new_object_collider(g_dev.p_physics_world, g_dev.p_selected_entity->p_physics_object, PHYSICS_COLLIDER_TYPE_sphere);
            } else if (g_dev.p_selected_entity->p_physics_object->_p_collider->type != PHYSICS_COLLIDER_TYPE_plane) {
                physics_collider_init(g_dev.p_selected_entity->p_physics_object->_p_collider, g_dev.p_selected_entity->p_physics_object->_p_collider->type + 1);

                if (g_dev.p_selected_entity->p_physics_object->_p_collider->type == PHYSICS_COLLIDER_TYPE_hull) {
                    darr_set_length(&g_dev.p_selected_entity->p_physics_object->_p_collider->as_hull.verts, g_dev.num_hull_points);
                    for (size_t i = 0; i < g_dev.num_hull_points; ++i) {
                        float* p_v = darr_get(&g_dev.p_selected_entity->p_physics_object->_p_collider->as_hull.verts, i);
                        vec3_set(&g_dev.p_hull_points[i * 3], p_v);
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
                p_new_scene_entity->p_physics_object = physics_world_new_object(g_dev.p_selected_entity->p_physics_object->_p_world, 0, g_dev.p_selected_entity->p_physics_object->_b_is_rigidbody);

                if (p_new_scene_entity->p_physics_object->_b_is_rigidbody) {
                    const struct physics_rigidbody* p_rigidbody = (const struct physics_rigidbody*)g_dev.p_selected_entity->p_physics_object;
                    struct physics_rigidbody* p_new_rigidbody = (struct physics_rigidbody*)p_new_scene_entity->p_physics_object;
                    *p_new_rigidbody = *p_rigidbody;
                    p_new_rigidbody->base._p_transform = &p_new_scene_entity->transform;
                }

                if (g_dev.p_selected_entity->p_physics_object->_p_collider) {
                    *p_new_scene_entity->p_physics_object->_p_collider = *g_dev.p_selected_entity->p_physics_object->_p_collider;
                }
            }

            set_selected_entity(p_new_scene_entity);

            log_msg(LOG_DEBUG, 0, "Scene entity copied\n");

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

void on_mouse_button(const void* p_event_data, void*) {
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

void on_mouse_move(const void* p_event_data, void*) {
    if (g_dev.pressed_mesh_id == 0 || g_dev.pressed_mesh_id > DEV_MESH_ID_CAPTURER_RESERVED_IDS) {
        return;
    }

    int mouse_client_coords[2];
    platform_window_get_mouse_client_coords(g_dev.p_platform_window, &mouse_client_coords[0], &mouse_client_coords[1]);

    float cursor_ray[3];
    platform_window_client_to_world_ray(g_dev.p_platform_window, g_dev.perspective_view_matrix, mouse_client_coords[0], mouse_client_coords[1], cursor_ray);
 
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

void draw_physics(void) {
    srand(117);

    for (size_t i = 0; i < g_dev.p_scene->_entities._length; ++i) {
        struct scene_entity* p_entity = *(struct scene_entity**)darr_get(&g_dev.p_scene->_entities, i);

        if (!p_entity->p_physics_object || !p_entity->p_physics_object->_p_collider) {
            continue;
        }

        if (p_entity->p_physics_object->_b_is_rigidbody) {
            float color[] = { randf(), randf(), randf() };
            glUniform3fv(g_dev.gl_program_flat_u_color, 1, color);
        } else {
            float color[] = { randf(), randf(), randf() };
            glUniform3fv(g_dev.gl_program_flat_u_color, 1, color);
        }

        float physics_collider_trs_matrix[16];
        compute_physics_collider_transform_matrix(p_entity->p_physics_object->_p_collider, p_entity->p_physics_object->_p_transform, physics_collider_trs_matrix);

        glUniformMatrix4fv(g_dev.gl_program_flat_u_model_matrix, 1, GL_FALSE, physics_collider_trs_matrix);
        gl_mesh_draw(&g_dev.gl_physics_collider_meshes[p_entity->p_physics_object->_p_collider->type]);
    }
}

void compute_physics_collider_transform_matrix(const struct physics_collider* p_collider, const struct transform* p_transform, float* p_collider_transform_matrix) {
    switch (p_collider->type) {
        case PHYSICS_COLLIDER_TYPE_sphere: {
            const float diameter = p_collider->as_sphere.radius * 2;
            matrix_make_uniform_scale(diameter * vec3_major(p_transform->world_scale), p_collider_transform_matrix);

            p_collider_transform_matrix[12] = p_transform->world_position[0];
            p_collider_transform_matrix[13] = p_transform->world_position[1];
            p_collider_transform_matrix[14] = p_transform->world_position[2];

            break;
        }

        case PHYSICS_COLLIDER_TYPE_capsule: {
            const float diameter = p_collider->as_capsule.radius * 2;
            matrix_make_uniform_scale(diameter * vec3_major(p_transform->world_scale), p_collider_transform_matrix);

            p_collider_transform_matrix[12] = p_transform->world_position[0];
            p_collider_transform_matrix[13] = p_transform->world_position[1];
            p_collider_transform_matrix[14] = p_transform->world_position[2];

            // todo: rotation
            break;
        }

        case PHYSICS_COLLIDER_TYPE_hull: {
            matrix_copy(p_transform->world_trs_matrix, p_collider_transform_matrix);
            break;
        }

        case PHYSICS_COLLIDER_TYPE_plane: {
            const struct physics_plane_collider* p_plane = (const struct physics_plane_collider*)p_collider;

            matrix_make_translation(p_transform->world_position[0], p_transform->world_position[1], p_transform->world_position[2], p_collider_transform_matrix);

            // todo: ignore scale, but combine transform rotation with normal direction
            break;
        }
    }
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
    
    glUniformMatrix4fv(g_dev.gl_program_flat_u_model_matrix, 1, GL_FALSE, g_dev.gizmos.gizmo_transform);

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
    const int b_highlight = (g_dev.pressed_mesh_id == 0 && g_dev.target_mesh_id == p_control->mesh_id_capturer_id) || (g_dev.pressed_mesh_id == p_control->mesh_id_capturer_id);
    glUniform3fv(g_dev.gl_program_flat_u_color, 1, b_highlight ? g_dev.gizmos.hovered_control_color : p_control->color);
    gl_mesh_draw(&p_control->gl_mesh);
}

int find_gizmo_control_plane_cursor_drag_intersection(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_pos) {
    float control_plane_d = -vec3_dot(p_control_plane_normal, &g_dev.gizmos.gizmo_transform[12]);

    if (!ray_plane_intersection(p_cursor_ray_origin, p_cursor_ray, p_control_plane_normal, control_plane_d, p_cursor_world_pos)) {
        return 0;
    }

    if (g_dev.b_is_dragging) {
        return 1;
    }
    
    vec3_set(p_cursor_world_pos, g_dev.gizmos.cursor_drag_last_world_pos);
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
        vec3_set(up, p_plane_norm_d);
    } else {
        vec3_set(side, p_plane_norm_d);
    }
}

void compute_gizmo_control_plane_cursor_drag_delta(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray, float* p_cursor_world_delta) {
    float cursor_ray_intersection[3];
    if (!find_gizmo_control_plane_cursor_drag_intersection(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, cursor_ray_intersection)) {
        vec3_clr(p_cursor_world_delta);
        return;
    }

    vec3_sub(cursor_ray_intersection, g_dev.gizmos.cursor_drag_last_world_pos, p_cursor_world_delta);
    
    vec3_set(cursor_ray_intersection, g_dev.gizmos.cursor_drag_last_world_pos);
}

float compute_gizmo_control_plane_cursor_drag_delta_angle(const float* p_control_plane_normal, const float* p_cursor_ray_origin, const float* p_cursor_ray) {
    float p0[3];
    float p1[3];

    if (!find_gizmo_control_plane_cursor_drag_intersection(p_control_plane_normal, p_cursor_ray_origin, p_cursor_ray, p1)) {
        return 0;
    }

    vec3_sub(g_dev.gizmos.cursor_drag_last_world_pos, &g_dev.gizmos.gizmo_transform[12], p0);
    vec3_set(p1, g_dev.gizmos.cursor_drag_last_world_pos);

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

void mesh_selector_render_pass(size_t framebuffer_width, size_t framebuffer_height, const float* p_projection_matrix, const float* p_view_matrix) {
    mesh_id_capturer_begin(&g_dev.mesh_id_capturer, framebuffer_width, framebuffer_height, p_projection_matrix, p_view_matrix);

    for (size_t i = 0; i < g_dev.p_scene->_entities._length; ++i) {
        struct scene_entity* p_entity = *(struct scene_entity**)darr_get(&g_dev.p_scene->_entities, i);

        const unsigned int mesh_id_capturer_entity_id = DEV_MESH_ID_CAPTURER_RESERVED_IDS + p_entity->_id;

        if (p_entity->p_mesh) {
            const struct static_mesh* p_mesh = p_entity->p_mesh->_asset._p_data;

            for (size_t j = 0; j < p_mesh->num_primitives; ++j) {
                const struct gl_mesh* p_gl_mesh = &p_mesh->p_gl_meshes[j];
                mesh_id_capturer_submit(&g_dev.mesh_id_capturer, p_gl_mesh, p_entity->transform.world_trs_matrix, mesh_id_capturer_entity_id);
            }
        }

        if (!g_dev.b_draw_physics || !p_entity->p_physics_object || !p_entity->p_physics_object->_p_collider) {
            continue;
        }

        float physics_collider_trs_matrix[16];
        compute_physics_collider_transform_matrix(p_entity->p_physics_object->_p_collider, p_entity->p_physics_object->_p_transform, physics_collider_trs_matrix);

        mesh_id_capturer_submit(&g_dev.mesh_id_capturer, &g_dev.gl_physics_collider_meshes[p_entity->p_physics_object->_p_collider->type], physics_collider_trs_matrix, mesh_id_capturer_entity_id);
    }
    
    if (g_dev.gizmos.p_target_transform) {
        glClear(GL_DEPTH_BUFFER_BIT);

        switch (g_dev.gizmos.active_type) {
            case GIZMO_TYPE_translate: {
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_t_x);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_t_y);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_t_z);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_t_xy);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_t_xz);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_t_yz);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_t_center);
                break;
            }
            
            case GIZMO_TYPE_rotate: {
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_r_x);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_r_y);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_r_z);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_r_center);
                break;
            }
            
            case GIZMO_TYPE_scale: {
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_s_x);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_s_y);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_s_z);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_s_xy);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_s_xz);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_s_yz);
                    mesh_selector_render_pass_submit_gizmo_control(&g_dev.gizmos.control_s_center);
                break;
            }
        }
    }
    
    int mouse_client_coords[2];
    platform_window_get_mouse_client_coords(g_dev.p_platform_window, &mouse_client_coords[0], &mouse_client_coords[1]);

    float mouse_position_normalized[2];
    platform_window_normalize_client_coords(g_dev.p_platform_window, mouse_client_coords[0], mouse_client_coords[1], &mouse_position_normalized[0], &mouse_position_normalized[1]);

    g_dev.target_mesh_id = mesh_id_capturer_query(&g_dev.mesh_id_capturer, mouse_position_normalized[0], mouse_position_normalized[1]);
}

void mesh_selector_render_pass_submit_gizmo_control(const struct gizmo_control* p_control) {
    mesh_id_capturer_submit(&g_dev.mesh_id_capturer, &p_control->gl_mesh, g_dev.gizmos.gizmo_transform, p_control->mesh_id_capturer_id);
} 

void draw_hull_DEBUGDEBUGDEBUG(void) {
    if (!g_dev.p_selected_entity || !g_dev.p_selected_entity->p_mesh) {
        return;
    }

    static struct he_mesh he_mesh;
    
    static struct mesh_primitive mesh_primitive;
    static struct mesh_primitive mesh_primitive_outline;
    
    static struct gl_mesh gl_mesh;
    static struct gl_mesh gl_mesh_outline;

    static struct scene_entity* p_old_selected_entity;

    if (g_dev.p_selected_entity != p_old_selected_entity) {
        p_old_selected_entity = g_dev.p_selected_entity;

        if (he_mesh.p_buffer) {
            quickhull_free(&he_mesh);
            mesh_factory_free_primitive(&mesh_primitive);
            gl_mesh_destroy(&gl_mesh);
            gl_mesh_destroy(&gl_mesh_outline);
        }

        quickhull_static_mesh((const struct static_mesh*)g_dev.p_selected_entity->p_mesh->_asset._p_data, &he_mesh);
        
        mesh_factory_make_from_halfedge_mesh(&he_mesh, &mesh_primitive, 0);
        gl_mesh_create(&gl_mesh, &mesh_primitive);
    
        mesh_factory_make_from_halfedge_mesh(&he_mesh, &mesh_primitive_outline, 1);
        gl_mesh_create(&gl_mesh_outline, &mesh_primitive_outline);
    }

    float matrix[16];
    matrix_make_identity(matrix);
    glUniformMatrix4fv(g_dev.gl_program_flat_u_model_matrix, 1, GL_FALSE, matrix);
    
    float color[] = { 1, 1, 1 };
    glUniform3fv(g_dev.gl_program_flat_u_color, 1, color);
    gl_mesh_draw(&gl_mesh);
    
    float color_wf[] = { 1, 0, 0 };
    glUniform3fv(g_dev.gl_program_flat_u_color, 1, color_wf);
    gl_mesh_draw(&gl_mesh_outline);
}