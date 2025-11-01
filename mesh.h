#ifndef _H__MESH
#define _H__MESH

#include <stdint.h>

enum vertex_attribute_type {
    VERTEX_ATTRIBUTE_TYPE_f32,
    VERTEX_ATTRIBUTE_TYPE_i8,
    VERTEX_ATTRIBUTE_TYPE_u8,
    VERTEX_ATTRIBUTE_TYPE_i16,
    VERTEX_ATTRIBUTE_TYPE_u16,
    VERTEX_ATTRIBUTE_TYPE_i32,
    VERTEX_ATTRIBUTE_TYPE_u32,
    VERTEX_ATTRIBUTE_TYPE_ni8,
    VERTEX_ATTRIBUTE_TYPE_nu8,
    VERTEX_ATTRIBUTE_TYPE_ni16,
    VERTEX_ATTRIBUTE_TYPE_nu16,
    VERTEX_ATTRIBUTE_TYPE_ni32,
    VERTEX_ATTRIBUTE_TYPE_nu32
};

struct vertex_buffer {
    void*  p_bytes;
    size_t size;
};

struct vertex_attribute_layout {
    size_t                     offset;
    size_t                     stride;
    size_t                     component_count;
    enum vertex_attribute_type component_type;
};

struct vertex_attribute {
    size_t                         index;
    size_t                         vertex_buffer_index;
    struct vertex_attribute_layout layout;
};

enum vertex_index_type {
    VERTEX_INDEX_TYPE_u8,
    VERTEX_INDEX_TYPE_u16,
    VERTEX_INDEX_TYPE_u32
};

struct vertex_index_buffer {
    void*                  p_bytes;
    size_t                 count;
    enum vertex_index_type type;
};

enum mesh_primitive_draw_mode {
    MESH_PRIMITIVE_DRAW_MODE_points,
    MESH_PRIMITIVE_DRAW_MODE_line_strip,
    MESH_PRIMITIVE_DRAW_MODE_line_loop,
    MESH_PRIMITIVE_DRAW_MODE_lines,
    MESH_PRIMITIVE_DRAW_MODE_triangle_strip,
    MESH_PRIMITIVE_DRAW_MODE_triangle_fan,
    MESH_PRIMITIVE_DRAW_MODE_triangles
};

struct mesh_primitive {
    struct vertex_buffer*           p_vertex_buffers;
    size_t                          num_vertex_buffers;
    struct vertex_attribute*        p_attributes;
    size_t                          num_attributes;
    size_t                          vertex_count;
    struct vertex_index_buffer      index_buffer;
    enum mesh_primitive_draw_mode   draw_mode;
    float                           bounds_min[3];
    float                           bounds_max[3];
};

size_t vertex_attribute_type_size(enum vertex_attribute_type type);
size_t vertex_attribute_layout_element_size(const struct vertex_attribute_layout* p_layout);

size_t vertex_index_type_size(enum vertex_index_type type);

void mesh_generate_normals(struct mesh_primitive* p_mesh_primitive, size_t positions_attribute_index, size_t normals_attribute_index);
void mesh_generate_tangents(struct mesh_primitive* p_mesh_primitive, size_t normals_attribute_index, size_t tangents_attribute_index);

#endif