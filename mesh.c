#include "mesh.h"
#include "vector.h"

size_t vertex_attribute_type_size(enum vertex_attribute_type type) {
    switch (type) {
        case VERTEX_ATTRIBUTE_TYPE_i8:
        case VERTEX_ATTRIBUTE_TYPE_u8:
        case VERTEX_ATTRIBUTE_TYPE_ni8:
        case VERTEX_ATTRIBUTE_TYPE_nu8:  return 1;
        case VERTEX_ATTRIBUTE_TYPE_i16:
        case VERTEX_ATTRIBUTE_TYPE_u16:
        case VERTEX_ATTRIBUTE_TYPE_ni16:
        case VERTEX_ATTRIBUTE_TYPE_nu16: return 2;
        case VERTEX_ATTRIBUTE_TYPE_f32:
        case VERTEX_ATTRIBUTE_TYPE_i32:
        case VERTEX_ATTRIBUTE_TYPE_u32:
        case VERTEX_ATTRIBUTE_TYPE_ni32:
        case VERTEX_ATTRIBUTE_TYPE_nu32: return 4;
    }
    return 0;
}

size_t vertex_attribute_layout_element_size(const struct vertex_attribute_layout* p_layout) {
    return vertex_attribute_type_size(p_layout->component_type) * p_layout->component_count;
}

size_t vertex_index_type_size(enum vertex_index_type type) {
    switch (type) {
        case VERTEX_INDEX_TYPE_u8:  return 1;
        case VERTEX_INDEX_TYPE_u16: return 2;
        case VERTEX_INDEX_TYPE_u32: return 4;
    }
    return 0;    
}

void mesh_generate_normals(struct mesh_primitive* p_mesh_primitive, size_t positions_attribute_index, size_t normals_attribute_index) {
    const struct vertex_attribute* p_attribute_positions = &p_mesh_primitive->p_attributes[positions_attribute_index];
    const struct vertex_attribute* p_attribute_normals = &p_mesh_primitive->p_attributes[normals_attribute_index];

    const struct vertex_buffer* p_vertex_buffer_positions = &p_mesh_primitive->p_vertex_buffers[p_attribute_positions->vertex_buffer_index];
    const struct vertex_buffer* p_vertex_buffer_normals = &p_mesh_primitive->p_vertex_buffers[p_attribute_normals->vertex_buffer_index];

    const void* p_positions = (unsigned char*)p_vertex_buffer_positions->p_bytes + p_attribute_positions->layout.offset;
    void* p_normals = (unsigned char*)p_vertex_buffer_normals->p_bytes + p_attribute_normals->layout.offset;

    float b_sub_a[3];
    float c_sub_a[3];

    if (p_mesh_primitive->index_buffer.p_bytes) {
        const size_t num_faces = p_mesh_primitive->index_buffer.count / 3;
        const size_t index_size = vertex_index_type_size(p_mesh_primitive->index_buffer.type);

        float n[3];

        for (size_t face_index = 0; face_index < num_faces; ++face_index) {
            size_t a_index;
            size_t b_index;
            size_t c_index;

            switch (p_mesh_primitive->index_buffer.type) {
                case VERTEX_INDEX_TYPE_u8: {
                    a_index = *((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * face_index * 3);
                    b_index = *((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * (face_index * 3 + 1));
                    c_index = *((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * (face_index * 3 + 2));
                    break;
                }

                case VERTEX_INDEX_TYPE_u16: {
                    a_index = *(unsigned short*)((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * face_index * 3);
                    b_index = *(unsigned short*)((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * (face_index * 3 + 1));
                    c_index = *(unsigned short*)((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * (face_index * 3 + 2));
                    break;
                }

                case VERTEX_INDEX_TYPE_u32: {
                    a_index = *(unsigned int*)((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * face_index * 3);
                    b_index = *(unsigned int*)((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * (face_index * 3 + 1));
                    c_index = *(unsigned int*)((unsigned char*)p_mesh_primitive->index_buffer.p_bytes + index_size * (face_index * 3 + 2));
                    break;
                }
            }


            const float* p_a_pos = (const void*)((const unsigned char*)p_positions + p_attribute_positions->layout.stride * a_index);
            const float* p_b_pos = (const void*)((const unsigned char*)p_positions + p_attribute_positions->layout.stride * b_index);
            const float* p_c_pos = (const void*)((const unsigned char*)p_positions + p_attribute_positions->layout.stride * c_index);

            vec3_sub(p_b_pos, p_a_pos, b_sub_a);
            vec3_sub(p_c_pos, p_a_pos, c_sub_a);
            vec3_cross(b_sub_a, c_sub_a, n);

            float* p_a_norm = (void*)((unsigned char*)p_normals + p_attribute_normals->layout.stride * a_index);
            float* p_b_norm = (void*)((unsigned char*)p_normals + p_attribute_normals->layout.stride * b_index);
            float* p_c_norm = (void*)((unsigned char*)p_normals + p_attribute_normals->layout.stride * c_index);

            vec3_add(n, p_a_norm, p_a_norm);
            vec3_add(n, p_b_norm, p_a_norm);
            vec3_add(n, p_c_norm, p_a_norm);
        }

        for (size_t v = 0; v < p_mesh_primitive->vertex_count; ++v) {
            float* p_n = (void*)((unsigned char*)p_normals + p_attribute_normals->layout.stride * v);
            vec3_norm(p_n, p_n);
        }
    } else {
        const size_t num_faces = p_mesh_primitive->vertex_count / 3;

        size_t offset;

        for (size_t face_index = 0; face_index < num_faces; ++face_index) {
            offset = p_attribute_positions->layout.stride * face_index * 3;

            const float* p_a_pos = (const void*)((const unsigned char*)p_positions + offset);
            const float* p_b_pos = (const void*)((const unsigned char*)p_positions + offset + p_attribute_positions->layout.stride * 1);
            const float* p_c_pos = (const void*)((const unsigned char*)p_positions + offset + p_attribute_positions->layout.stride * 2);
            
            vec3_sub(p_b_pos, p_a_pos, b_sub_a);
            vec3_sub(p_c_pos, p_a_pos, c_sub_a);

            offset = p_attribute_normals->layout.stride * face_index * 3;

            float* p_a_norm = (void*)((unsigned char*)p_normals + offset);
            float* p_b_norm = (void*)((unsigned char*)p_normals + offset + p_attribute_normals->layout.stride * 1);
            float* p_c_norm = (void*)((unsigned char*)p_normals + offset + p_attribute_normals->layout.stride * 2);

            vec3_cross(b_sub_a, c_sub_a, p_a_norm);
            vec3_norm(p_a_norm, p_a_norm);

            vec3_set(p_a_norm, p_b_norm);
            vec3_set(p_a_norm, p_c_norm);
        }
    }
}

void mesh_generate_tangents(struct mesh_primitive* p_mesh_primitive, size_t normals_attribute_index, size_t tangents_attribute_index) {

}