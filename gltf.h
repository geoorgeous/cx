#ifndef _H__GLTF
#define _H__GLTF

#include <stdint.h>

#define GLTF_SUCCESS 0
#define GLTF_ERROR_UNKNOWN -1
#define GLTF_ERROR_MEMORY 1
#define GLTF_ERROR_FILE 2
#define GLTF_ERROR_JSON_PARSING 3
#define GLTF_ERROR_OBJECT_MISSING_REQUIRED_PROPERTY 4
#define GLTF_ERROR_BAD_TYPE 5
#define GLTF_ERROR_BAD_VALUE 6
#define GLTF_ERROR_BAD_URI 7

#define GLTF_GL_ARRAY_BUFFER           34962
#define GLTF_GL_ELEMENT_ARRAY_BUFFER   34963
#define GLTF_GL_BYTE                   5120
#define GLTF_GL_UNSIGNED_BYTE          5121
#define GLTF_GL_SHORT                  5122
#define GLTF_GL_UNSIGNED_SHORT         5123
#define GLTF_GL_UNSIGNED_INT           5125
#define GLTF_GL_FLOAT                  5126
#define GLTF_GL_NEAREST                9728
#define GLTF_GL_LINEAR                 9729
#define GLTF_GL_NEAREST_MIPMAP_NEAREST 9984
#define GLTF_GL_LINEAR_MIPMAP_NEAREST  9985
#define GLTF_GL_NEAREST_MIPMAP_LINEAR  9986
#define GLTF_GL_LINEAR_MIPMAP_LINEAR   9987
#define GLTF_GL_CLAMP_TO_EDGE          33071
#define GLTF_GL_MIRRORED_REPEAT        33648
#define GLTF_GL_REPEAT                 10497

#define GLTF_MAX_STRING_LEN 256

#define GLTF_INVALID_INDEX SIZE_MAX

typedef size_t gltf_index;

struct gltf_info {
    char*   s_version;
    char*   s_version_min;
    char*   s_generator;
    char*   s_copyright;
    char**  p_extensions;
    size_t  num_extensions;
    size_t* p_extensions_required;
    size_t  num_extensions_required;
};

struct gltf_buffer {
    size_t byte_length;
    void*  p_bytes;
};

enum gltf_buffer_view_target {
    GLTF_BUFFER_VIEW_TARGET_array_buffer         = GLTF_GL_ARRAY_BUFFER,
    GLTF_BUFFER_VIEW_TARGET_element_array_buffer = GLTF_GL_ELEMENT_ARRAY_BUFFER
};

struct gltf_buffer_view {
    gltf_index                   buffer_index;
    size_t                       byte_offset;
    size_t                       byte_length;
    size_t                       byte_stride;
    enum gltf_buffer_view_target target;
};

enum gltf_accessor_component_type {
    GLTF_ACCESSOR_COMPONENT_TYPE_byte           = GLTF_GL_BYTE,
    GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_byte  = GLTF_GL_UNSIGNED_BYTE,
    GLTF_ACCESSOR_COMPONENT_TYPE_short          = GLTF_GL_SHORT,
    GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_short = GLTF_GL_UNSIGNED_SHORT,
    GLTF_ACCESSOR_COMPONENT_TYPE_unsigned_int   = GLTF_GL_UNSIGNED_INT,
    GLTF_ACCESSOR_COMPONENT_TYPE_float          = GLTF_GL_FLOAT
};

enum gltf_accessor_type {
    GLTF_ACCESSOR_TYPE_scalar,
    GLTF_ACCESSOR_TYPE_vec2,
    GLTF_ACCESSOR_TYPE_vec3,
    GLTF_ACCESSOR_TYPE_vec4,
    GLTF_ACCESSOR_TYPE_mat2,
    GLTF_ACCESSOR_TYPE_mat3,
    GLTF_ACCESSOR_TYPE_mat4
};

struct gltf_accessor {
    gltf_index                        buffer_view_index;
    size_t                            byte_offset;
    enum gltf_accessor_component_type component_type;
    int                               b_normalized;
    unsigned int                      count;
    enum gltf_accessor_type           type;
    float                             max[16];
    float                             min[16];
};

struct gltf_image {
    int b_uri_source;
    union {
        struct {
            void*  p_data;
            size_t size;
        } uri;
        gltf_index buffer_view_index;
    } source;
    char s_mime_type[GLTF_MAX_STRING_LEN];
};

enum gltf_sampler_mag_filter {
    GLTF_SAMPLER_MAG_FILTER_nearest = GLTF_GL_NEAREST,
    GLTF_SAMPLER_MAG_FILTER_linear  = GLTF_GL_LINEAR
};

enum gltf_sampler_min_filter {
    GLTF_SAMPLER_MIN_FILTER_nearest                = GLTF_GL_NEAREST,
    GLTF_SAMPLER_MIN_FILTER_linear                 = GLTF_GL_LINEAR,
    GLTF_SAMPLER_MIN_FILTER_nearest_mipmap_nearest = GLTF_GL_NEAREST_MIPMAP_NEAREST,
    GLTF_SAMPLER_MIN_FILTER_linear_mipmap_nearest  = GLTF_GL_LINEAR_MIPMAP_NEAREST,
    GLTF_SAMPLER_MIN_FILTER_nearest_mipmap_linear  = GLTF_GL_NEAREST_MIPMAP_LINEAR,
    GLTF_SAMPLER_MIN_FILTER_linear_mipmap_linear   = GLTF_GL_LINEAR_MIPMAP_LINEAR
};

enum gltf_sampler_wrap {
    GLTF_SAMPLER_WRAP_clamp_to_edge   = GLTF_GL_CLAMP_TO_EDGE,
    GLTF_SAMPLER_WRAP_mirrored_repeat = GLTF_GL_MIRRORED_REPEAT,
    GLTF_SAMPLER_WRAP_repeat          = GLTF_GL_REPEAT
};

struct gltf_texture {
    gltf_index                   source_image_index;
    enum gltf_sampler_mag_filter sampler_mag_filter;
    enum gltf_sampler_min_filter sampler_min_filter;
    enum gltf_sampler_wrap       sampler_wrap_s;
    enum gltf_sampler_wrap       sampler_wrap_t;
};

struct gltf_material_texture_info {
    gltf_index source_texture_index;
    size_t     tex_coord_set_index;
};

enum gltf_material_alpha_mode {
    GLTF_MATERIAL_ALPHA_MODE_opaque,
    GLTF_MATERIAL_ALPHA_MODE_mask,
    GLTF_MATERIAL_ALPHA_MODE_blend
};

struct gltf_material {
    float                             pbr_base_color_factor[4];
    struct gltf_material_texture_info pbr_base_color_texture;
    float                             pbr_metallic_factor;
    float                             pbr_roughness_factor;
    struct gltf_material_texture_info pbr_metallic_roughness_texture;
    struct gltf_material_texture_info normal_texture;
    struct gltf_material_texture_info occlusion_texture;
    struct gltf_material_texture_info emissive_texture;
    float                             emissive_factor[3];
    enum gltf_material_alpha_mode     alpha_mode;
    float                             mask_alpha_cutoff;
    int                               b_double_sided;
};

enum gltf_mesh_vertex_attribute {
    GLTF_MESH_VERTEX_ATTRIBUTE_position,
    GLTF_MESH_VERTEX_ATTRIBUTE_normal,
    GLTF_MESH_VERTEX_ATTRIBUTE_tangent,
    GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_0,
    GLTF_MESH_VERTEX_ATTRIBUTE_texcoord_1,
    GLTF_MESH_VERTEX_ATTRIBUTE_color,
    GLTF_MESH_VERTEX_ATTRIBUTE_joints,
    GLTF_MESH_VERTEX_ATTRIBUTE_weights,
    GLTF_MESH_VERTEX_ATTRIBUTE__MAX
};

enum gltf_mesh_primitive_mode {
    GLTF_MESH_PRIMITIVE_MODE_points,
    GLTF_MESH_PRIMITIVE_MODE_lines,
    GLTF_MESH_PRIMITIVE_MODE_line_loop,
    GLTF_MESH_PRIMITIVE_MODE_line_strip,
    GLTF_MESH_PRIMITIVE_MODE_triangles,
    GLTF_MESH_PRIMITIVE_MODE_triangle_strip,
    GLTF_MESH_PRIMITIVE_MODE_triangle_fan
};

struct gltf_mesh_primitive {
    gltf_index                    vertex_attribute_accessors_indices[GLTF_MESH_VERTEX_ATTRIBUTE__MAX];
    gltf_index                    vertex_indices_accessor_index;
    gltf_index                    material_index;
    enum gltf_mesh_primitive_mode mode;
};

struct gltf_mesh {
    struct gltf_mesh_primitive* p_primitives;
    size_t                      num_primitives;
};

struct gltf_skin {
    gltf_index  inverse_bind_matrices_accessor_index;
    gltf_index* p_joints_indices;
    size_t      num_joints;
};

enum gltf_animation_sampler_interpolation {
    GLTF_ANIMATION_SAMPLER_INTERPOLATION_linear,
    GLTF_ANIMATION_SAMPLER_INTERPOLATION_step,
    GLTF_ANIMATION_SAMPLER_INTERPOLATION_cubic_splines
};

struct gltf_animation_sampler {
    gltf_index                                input_accessor_index;
    enum gltf_animation_sampler_interpolation interpolation;
    gltf_index                                output_accessor_index;
};

enum gltf_animation_channel_target_path {
    GLTF_ANIMATION_CHANNEL_TARGET_PATH_translation,
    GLTF_ANIMATION_CHANNEL_TARGET_PATH_rotation,
    GLTF_ANIMATION_CHANNEL_TARGET_PATH_scale
};

struct gltf_animation_channel {
    gltf_index                              source_sampler_index;
    gltf_index                              target_node_index;
    enum gltf_animation_channel_target_path target_path;
};

struct gltf_animation {
    struct gltf_animation_channel* p_channels;
    size_t                         num_channels;
    struct gltf_animation_sampler* p_samplers;
    size_t                         num_samplers;
};

struct gltf_node {
    gltf_index* p_children_indices;
    size_t      num_children;
    gltf_index  skin_index;
    float       matrix[16];
    gltf_index  mesh_index;
    char        s_name[GLTF_MAX_STRING_LEN];
    int         b_is_joint;
};

struct gltf_scene {
    gltf_index* p_root_nodes_indices;
    size_t      num_root_nodes;
};

struct gltf {
    struct gltf_info         info;
    struct gltf_buffer*      p_buffers;
    size_t                   num_buffers;
    struct gltf_buffer_view* p_buffer_views;
    size_t                   num_buffer_views;
    struct gltf_accessor*    p_accessors;
    size_t                   num_accessors;
    struct gltf_image*       p_images;
    size_t                   num_images;
    struct gltf_texture*     p_textures;
    size_t                   num_textures;
    struct gltf_material*    p_materials;
    size_t                   num_materials;
    struct gltf_mesh*        p_meshes;
    size_t                   num_meshes;
    struct gltf_skin*        p_skins;
    size_t                   num_skins;
    struct gltf_animation*   p_animations;
    size_t                   num_animations;
    struct gltf_node*        p_nodes;
    size_t                   num_nodes;
    struct gltf_scene*       p_scenes;
    size_t                   num_scenes;
    gltf_index               default_scene_index;
};

int  gltf_load_from_file(const char* s_filename, struct gltf* p_gltf);
void gltf_free(struct gltf* p_gltf);

#endif