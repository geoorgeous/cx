#include "gl_program.h"
#include "gl_mesh.h"
#include "logging.h"
#include "matrix.h"
#include "mesh_factory.h"
#include "mesh.h"
#include "skeletal_animation_debug.h"
#include "skeleton.h"
#include "vector.h"

static struct {
    int b_initialized;
    struct gl_mesh gl_joint_mesh;
    struct gl_mesh gl_bone_mesh;
    struct gl_program gl_program;
} g_rendering;

static void init_rendering(void);
static void debug_draw_skeleton_joint_hierarchy(const struct skeleton* p_skeleton, const float* p_parent_transform, const struct skeleton_joint* p_joint);
static void debug_draw_skeleton_joint(const float* p_transform, const float* p_color);
static void debug_draw_skeleton_bone(const float* p_transform_a, const float* p_transform_b, const float* p_color);

void debug_draw_skeleton(const struct skeleton* p_skeleton, const float* p_projection_matrix, const float* p_view_matrix, const float* p_model_matrix) {
    if (!g_rendering.b_initialized) {
        init_rendering();
    }
    
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(g_rendering.gl_program.gl_handle);

    GLuint gl_uniform_location;
    
    gl_uniform_location = glGetUniformLocation(g_rendering.gl_program.gl_handle, "u_projection_matrix");
    glUniformMatrix4fv(gl_uniform_location, 1, GL_FALSE, p_projection_matrix);
    
    gl_uniform_location = glGetUniformLocation(g_rendering.gl_program.gl_handle, "u_view_matrix");
    glUniformMatrix4fv(gl_uniform_location, 1, GL_FALSE, p_view_matrix);

    for (size_t i = 0; i < p_skeleton->num_root_joints; ++i) {
        const struct skeleton_joint* p_root_joint = &p_skeleton->p_joints[p_skeleton->p_root_joints_indices[i]];
        debug_draw_skeleton_joint_hierarchy(p_skeleton, p_model_matrix, p_root_joint);
    }
}

static void init_rendering(void) {
    struct mesh_primitive joint_mesh;
    mesh_factory_make_uv_sphere_primitive(0.0175f, 8, &joint_mesh);
    gl_mesh_create(&g_rendering.gl_joint_mesh, &joint_mesh);

    const float hw = 0.1f;
    const float zm = hw * 2;
    float bone_mesh_vertices[] = {
         0,   0,  0,
         hw,  hw, zm,
         hw, -hw, zm,
        -hw, -hw, zm,
        -hw,  hw, zm,
         0,   0,  1
    };

    int bone_mesh_indices[] = {
        0, 2, 1,
        0, 3, 2,
        0, 4, 3,
        0, 1, 4,
        5, 1, 2,
        5, 2, 3,
        5, 3, 4,
        5, 4, 1
    };

    struct vertex_buffer bone_vertex_buffer = {
        .p_bytes = bone_mesh_vertices,
        .size = sizeof(bone_mesh_vertices)
    };

    struct vertex_attribute bone_mesh_vertex_attribute = {
        .index = 0,
        .vertex_buffer_index = 0,
        .layout = {
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };

    struct vertex_index_buffer bone_mesh_index_buffer = {
        .p_bytes = bone_mesh_indices,
        .count = sizeof(bone_mesh_indices) / sizeof(*bone_mesh_indices),
        .type = VERTEX_INDEX_TYPE_u32
    };

    struct mesh_primitive bone_mesh = {
        .p_vertex_buffers = &bone_vertex_buffer,
        .num_vertex_buffers = 1,
        .p_attributes = &bone_mesh_vertex_attribute,
        .num_attributes = 1,
        .vertex_count = 6,
        .index_buffer = bone_mesh_index_buffer,
        .draw_mode = MESH_PRIMITIVE_DRAW_MODE_line_strip
    };
    
    gl_mesh_create(&g_rendering.gl_bone_mesh, &bone_mesh);

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

    gl_program_create(&g_rendering.gl_program);
    gl_program_attach_shader(&g_rendering.gl_program, &gl_vertex_shader);
    gl_program_attach_shader(&g_rendering.gl_program, &gl_fragment_shader);
    gl_program_link(&g_rendering.gl_program);

    gl_shader_destroy(&gl_vertex_shader);
    gl_shader_destroy(&gl_fragment_shader);

    g_rendering.b_initialized = 1;
}

void debug_draw_skeleton_joint_hierarchy(const struct skeleton* p_skeleton, const float* p_parent_transform, const struct skeleton_joint* p_joint) {
    float joint_world_transform[16];
    matrix_multiply(p_parent_transform, p_joint->transform, joint_world_transform);

    for (size_t i = 0; i < p_joint->num_children; ++i) {
        const struct skeleton_joint* p_child_joint = &p_skeleton->p_joints[p_joint->p_children_indices[i]];
        
        debug_draw_skeleton_joint_hierarchy(p_skeleton, joint_world_transform, p_child_joint);
        
        float bone_color[] = { 0.27f, 0.51f, 0.71f };
        debug_draw_skeleton_bone(joint_world_transform, p_child_joint->transform, bone_color);
    }

    float color[] = { 0.8f, 0.8f, 0.8f };
    debug_draw_skeleton_joint(joint_world_transform, color);
}

void debug_draw_skeleton_joint(const float* p_transform, const float* p_color) {
    const float inverse_scale_x = 1.0f / vec3_len(&p_transform[0]);
    const float inverse_scale_y = 1.0f / vec3_len(&p_transform[4]);
    const float inverse_scale_z = 1.0f / vec3_len(&p_transform[8]);
    float fixed_scale_transform[16];
    matrix_make_scale(inverse_scale_x, inverse_scale_y, inverse_scale_z, fixed_scale_transform);
    matrix_multiply(p_transform, fixed_scale_transform, fixed_scale_transform);

    GLuint gl_uniform_location = glGetUniformLocation(g_rendering.gl_program.gl_handle, "u_model_matrix");
    glUniformMatrix4fv(gl_uniform_location, 1, GL_FALSE, fixed_scale_transform);
    
    gl_uniform_location = glGetUniformLocation(g_rendering.gl_program.gl_handle, "u_color");
    glUniform3fv(gl_uniform_location, 1, p_color);

    gl_mesh_draw(&g_rendering.gl_joint_mesh);
}

void debug_draw_skeleton_bone(const float* p_transform_a, const float* p_transform_b, const float* p_color) {
    float m_trs[16];

    // Bone scale

    float transform_b_world[16];
    matrix_multiply(p_transform_a, p_transform_b, transform_b_world);

    float a_to_b[3];
    vec3_sub(&transform_b_world[12], &p_transform_a[12], a_to_b);

    float scale = vec3_len(a_to_b);
    matrix_make_scale(scale, scale, scale, m_trs);
    
    // Bone rotation

    vec3_norm(a_to_b, a_to_b);

    float up[] = { 0, 1, 0 };

    float xaxis[3];
    vec3_cross(up, a_to_b, xaxis);
    vec3_norm(xaxis, xaxis);

    float yaxis[3];
    vec3_cross(a_to_b, xaxis, yaxis);
    vec3_norm(yaxis, yaxis);

    float m_r[] = {
        xaxis[0], xaxis[1], xaxis[2], 0,
        yaxis[0], yaxis[1], yaxis[2], 0,
        a_to_b[0], a_to_b[1], a_to_b[2], 0,
        0, 0, 0, 1
    };
    
    matrix_multiply(m_r, m_trs, m_trs);

    // Bone translation
    
    float m_t[16];
    matrix_make_translation(p_transform_a[12], p_transform_a[13], p_transform_a[14], m_t);
    matrix_multiply(m_t, m_trs, m_trs);

    GLuint gl_uniform_location = glGetUniformLocation(g_rendering.gl_program.gl_handle, "u_model_matrix");
    glUniformMatrix4fv(gl_uniform_location, 1, GL_FALSE, m_trs);
    
    gl_uniform_location = glGetUniformLocation(g_rendering.gl_program.gl_handle, "u_color");
    glUniform3fv(gl_uniform_location, 1, p_color);

    gl_mesh_draw(&g_rendering.gl_bone_mesh);
}