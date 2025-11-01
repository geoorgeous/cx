#include "gl_mesh.h"
#include "mesh.h"

int is_vertex_attribute_type_float(enum vertex_attribute_type vertex_attribute_type);
GLenum vertex_attribute_type_to_glenum(enum vertex_attribute_type vertex_attribute_type);
int is_vertex_attribute_type_normalized(enum vertex_attribute_type vertex_attribute_type);
GLenum index_type_to_glenum(enum vertex_index_type vertex_index_type);

void gl_mesh_create(struct gl_mesh* p_gl_mesh, const struct mesh_primitive* p_mesh_primitive) {
    *p_gl_mesh = (struct gl_mesh){0};

    glGenVertexArrays(1, &p_gl_mesh->_gl_vao);
    glBindVertexArray(p_gl_mesh->_gl_vao);

    p_gl_mesh->_gl_vbos_len = p_mesh_primitive->num_vertex_buffers;
    glGenBuffers(p_gl_mesh->_gl_vbos_len, p_gl_mesh->_gl_vbos);

    for (size_t i = 0; i < p_mesh_primitive->num_vertex_buffers; ++i) {
        const struct vertex_buffer* p_vertex_buffer = &p_mesh_primitive->p_vertex_buffers[i];

        glBindBuffer(GL_ARRAY_BUFFER, p_gl_mesh->_gl_vbos[i]);

        glBufferData(GL_ARRAY_BUFFER, p_vertex_buffer->size, p_vertex_buffer->p_bytes, GL_STATIC_DRAW);
    }

    for (size_t i = 0; i < p_mesh_primitive->num_attributes; ++i) {
        const struct vertex_attribute* p_attribute = &p_mesh_primitive->p_attributes[i];

        glBindBuffer(GL_ARRAY_BUFFER, p_gl_mesh->_gl_vbos[p_attribute->vertex_buffer_index]);

        if (is_vertex_attribute_type_float(p_attribute->layout.component_type)) {
            glVertexAttribPointer((GLuint)p_attribute->index,
                (GLint)p_attribute->layout.component_count,
                vertex_attribute_type_to_glenum(p_attribute->layout.component_type),
                (GLboolean)is_vertex_attribute_type_normalized(p_attribute->layout.component_type),
                (GLsizei)p_attribute->layout.stride,
                (void*)(GLsizeiptr)p_attribute->layout.offset
            );
        } else {
            glVertexAttribIPointer((GLuint)p_attribute->index,
                (GLint)p_attribute->layout.component_count,
                vertex_attribute_type_to_glenum(p_attribute->layout.component_type),
                (GLsizei)p_attribute->layout.stride,
                (void*)(GLsizeiptr)p_attribute->layout.offset
            );
        }
        
        glEnableVertexAttribArray(p_attribute->index);
    }

    if (p_mesh_primitive->index_buffer.p_bytes) {
        glGenBuffers(1, &p_gl_mesh->_gl_ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_gl_mesh->_gl_ibo);

        GLsizei index_size;
        switch(p_mesh_primitive->index_buffer.type) {
            case VERTEX_INDEX_TYPE_u8:  index_size = 1; break;
            case VERTEX_INDEX_TYPE_u16: index_size = 2; break;
            case VERTEX_INDEX_TYPE_u32: index_size = 4; break;
        }

        const GLsizei ibo_size = index_size * p_mesh_primitive->index_buffer.count;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo_size, p_mesh_primitive->index_buffer.p_bytes, GL_STATIC_DRAW);

        p_gl_mesh->_gl_ibo_type = index_type_to_glenum(p_mesh_primitive->index_buffer.type);
        p_gl_mesh->_num_elements = p_mesh_primitive->index_buffer.count;
    } else {
        p_gl_mesh->_num_elements = p_mesh_primitive->vertex_count;
    }

    switch (p_mesh_primitive->draw_mode) {
        case MESH_PRIMITIVE_DRAW_MODE_points:         p_gl_mesh->_gl_draw_mode = GL_POINTS; break;
        case MESH_PRIMITIVE_DRAW_MODE_line_strip:     p_gl_mesh->_gl_draw_mode = GL_LINE_STRIP; break;
        case MESH_PRIMITIVE_DRAW_MODE_line_loop:      p_gl_mesh->_gl_draw_mode = GL_LINE_LOOP; break;
        case MESH_PRIMITIVE_DRAW_MODE_lines:          p_gl_mesh->_gl_draw_mode = GL_LINES; break;
        case MESH_PRIMITIVE_DRAW_MODE_triangle_strip: p_gl_mesh->_gl_draw_mode = GL_TRIANGLE_STRIP; break;
        case MESH_PRIMITIVE_DRAW_MODE_triangle_fan:   p_gl_mesh->_gl_draw_mode = GL_TRIANGLE_FAN; break;
        case MESH_PRIMITIVE_DRAW_MODE_triangles:      p_gl_mesh->_gl_draw_mode = GL_TRIANGLES; break;
    }

    memcpy(p_gl_mesh->_bounds_min, p_mesh_primitive->bounds_min, sizeof(p_gl_mesh->_bounds_min));
    memcpy(p_gl_mesh->_bounds_max, p_mesh_primitive->bounds_max, sizeof(p_gl_mesh->_bounds_max));
}

void gl_mesh_destroy(struct gl_mesh* p_gl_mesh) {
    glDeleteBuffers(p_gl_mesh->_gl_vbos_len, p_gl_mesh->_gl_vbos);
    glDeleteBuffers(1, &p_gl_mesh->_gl_ibo);
    glDeleteVertexArrays(1, &p_gl_mesh->_gl_vao);
    *p_gl_mesh = (struct gl_mesh){0};
}

void gl_mesh_draw(const struct gl_mesh* p_gl_mesh) {
    glBindVertexArray(p_gl_mesh->_gl_vao);

    if (p_gl_mesh->_gl_ibo) {
        glDrawElements(p_gl_mesh->_gl_draw_mode, p_gl_mesh->_num_elements, p_gl_mesh->_gl_ibo_type, 0);
    } else {
        glDrawArrays(p_gl_mesh->_gl_draw_mode, 0, p_gl_mesh->_num_elements);
    }
}

int is_vertex_attribute_type_float(enum vertex_attribute_type vertex_attribute_type) {
    return
        vertex_attribute_type == VERTEX_ATTRIBUTE_TYPE_f32 ||
        is_vertex_attribute_type_normalized(vertex_attribute_type);
}

GLenum vertex_attribute_type_to_glenum(enum vertex_attribute_type vertex_attribute_type) {
    switch (vertex_attribute_type) {
        case VERTEX_ATTRIBUTE_TYPE_f32:  return GL_FLOAT;

        case VERTEX_ATTRIBUTE_TYPE_i8: 
        case VERTEX_ATTRIBUTE_TYPE_ni8:  return GL_BYTE;

        case VERTEX_ATTRIBUTE_TYPE_u8:
        case VERTEX_ATTRIBUTE_TYPE_nu8:  return GL_UNSIGNED_BYTE;

        case VERTEX_ATTRIBUTE_TYPE_i16:
        case VERTEX_ATTRIBUTE_TYPE_ni16: return GL_SHORT;

        case VERTEX_ATTRIBUTE_TYPE_u16:
        case VERTEX_ATTRIBUTE_TYPE_nu16: return GL_UNSIGNED_SHORT;

        case VERTEX_ATTRIBUTE_TYPE_i32:
        case VERTEX_ATTRIBUTE_TYPE_ni32: return GL_INT;

        case VERTEX_ATTRIBUTE_TYPE_u32:
        case VERTEX_ATTRIBUTE_TYPE_nu32: return GL_UNSIGNED_INT;
    }

    return GL_NONE;
}

int is_vertex_attribute_type_normalized(enum vertex_attribute_type vertex_attribute_type) {
    if (vertex_attribute_type >= VERTEX_ATTRIBUTE_TYPE_ni8 &&
        vertex_attribute_type <= VERTEX_ATTRIBUTE_TYPE_nu32) {
        return 1;
    }
    return 0;
}

GLenum index_type_to_glenum(enum vertex_index_type vertex_index_type) {
    switch (vertex_index_type) {
        case VERTEX_INDEX_TYPE_u8: return GL_UNSIGNED_BYTE;
        case VERTEX_INDEX_TYPE_u16: return GL_UNSIGNED_SHORT;
        case VERTEX_INDEX_TYPE_u32: return GL_UNSIGNED_INT;
    }

    return GL_NONE;
}