#ifndef CX_MESH_DATA_H
#define CX_MESH_DATA_H

#include <stddef.h>
#include <stdint.h>

#include "cx_hash.h"

enum cx_mesh_vertex_attribute_type {
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_i8,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_u8,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_i16,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_u16,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_i32,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_u32,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_ni8,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_nu8,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_ni16,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_nu16,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_ni32,
    CX_MESH_VERTEX_ATTRIBUTE_TYPE_nu32
};

struct cx_mesh_vertex_buffer {
    void*  p_bytes;
    size_t size;
};

struct cx_mesh_vertex_attribute_format {
	enum cx_mesh_vertex_attribute_type type;
	size_t count;
};

struct cx_mesh_vertex_attribute_layout {
    size_t offset;
    size_t stride;
};

struct cx_mesh_vertex_attribute {
    size_t index;
    size_t vertex_buffer_index;
	struct cx_mesh_vertex_attribute_format format;
    struct cx_mesh_vertex_attribute_layout layout;
};

enum cx_mesh_vertex_index_type {
	CX_MESH_VERTEX_INDEX_TYPE_none,
    CX_MESH_VERTEX_INDEX_TYPE_u8,
    CX_MESH_VERTEX_INDEX_TYPE_u16,
    CX_MESH_VERTEX_INDEX_TYPE_u32
};

struct cx_mesh_vertex_index_buffer {
    void*  p_bytes;
    size_t count;
};

enum cx_mesh_draw_mode {
    CX_MESH_DRAW_MODE_points,
    CX_MESH_DRAW_MODE_line_strip,
    CX_MESH_DRAW_MODE_line_loop,
    CX_MESH_DRAW_MODE_lines,
    CX_MESH_DRAW_MODE_triangle_strip,
    CX_MESH_DRAW_MODE_triangle_fan,
    CX_MESH_DRAW_MODE_triangles
};

struct cx_mesh_data_layout {
	size_t num_vertex_buffers;
	struct cx_mesh_vertex_attribute* p_attributes;
	size_t num_attributes;
	enum cx_mesh_vertex_index_type index_type;
	enum cx_mesh_draw_mode draw_mode;
};

struct cx_mesh_data {
	struct cx_mesh_data_layout layout;
    struct cx_mesh_vertex_buffer* p_vertex_buffers;
    size_t vertex_count;
    struct cx_mesh_vertex_index_buffer index_buffer;
    float bounds_min[3];
    float bounds_max[3];
};

size_t cx_mesh_vertex_attribute_type_size(enum cx_mesh_vertex_attribute_type type);

size_t cx_mesh_vertex_attribute_format_size(const struct cx_mesh_vertex_attribute_format* p_format);

size_t cx_mesh_vertex_index_type_size(enum cx_mesh_vertex_index_type type);

static inline uint64_t cx_mesh_data_layout_hash(const struct cx_mesh_data_layout* p_layout) {
	uint64_t hash = cx_hash_init();
	hash = cx_hash_u8(hash, (uint8_t)p_layout->num_vertex_buffers);
	hash = cx_hash_u8(hash, (uint8_t)p_layout->index_type);
	hash = cx_hash_u8(hash, (uint8_t)p_layout->num_vertex_buffers);
	hash = cx_hash_u8(hash, (uint8_t)p_layout->num_attributes);
	for (size_t i = 0; i < p_layout->num_attributes; ++i) {
		hash = cx_hash_u32(hash, (uint32_t)p_layout->p_attributes[i].index);
		hash = cx_hash_u32(hash, (uint32_t)p_layout->p_attributes[i].vertex_buffer_index);
		hash = cx_hash_u8(hash, (uint8_t)p_layout->p_attributes[i].format.type);
		hash = cx_hash_u8(hash, (uint8_t)p_layout->p_attributes[i].format.count);
		hash = cx_hash_u32(hash, (uint32_t)p_layout->p_attributes[i].layout.offset);
		hash = cx_hash_u32(hash, (uint32_t)p_layout->p_attributes[i].layout.stride);
	}
	return hash;
}

void cx_mesh_data_generate_normals(
	struct cx_mesh_data* p_mesh_data,
	size_t positions_attribute_index,
	size_t normals_attribute_index);

void cx_mesh_data_generate_tangents(
	struct cx_mesh_data* p_mesh_data,
	size_t normals_attribute_index,
	size_t tangents_attribute_index);

static inline int is_vertex_attribute_type_normalized(enum cx_mesh_vertex_attribute_type vertex_attribute_type) {
    if (vertex_attribute_type >= CX_MESH_VERTEX_ATTRIBUTE_TYPE_ni8 &&
        vertex_attribute_type <= CX_MESH_VERTEX_ATTRIBUTE_TYPE_nu32) {
        return 1;
    }
    return 0;
}

static inline int is_vertex_attribute_type_float(enum cx_mesh_vertex_attribute_type vertex_attribute_type) {
    return
        vertex_attribute_type == CX_MESH_VERTEX_ATTRIBUTE_TYPE_f32 ||
        is_vertex_attribute_type_normalized(vertex_attribute_type);
}

#endif
