#include <string.h>

#include "cx_gfx_mesh.h"
#include "gl.h"
#include "mesh.h"

static const GLenum gl_vertex_attr_type_table[] = {
	GL_FLOAT,
	GL_BYTE,
	GL_UNSIGNED_BYTE,
	GL_SHORT,
	GL_UNSIGNED_SHORT,
	GL_INT,
	GL_UNSIGNED_INT,
	GL_BYTE,
	GL_UNSIGNED_BYTE,
	GL_SHORT,
	GL_UNSIGNED_SHORT,
	GL_INT,
	GL_UNSIGNED_INT
};

static const GLenum gl_index_type_table[] = {
	GL_UNSIGNED_BYTE,
	GL_UNSIGNED_SHORT,
	GL_UNSIGNED_INT
};

static const GLenum gl_draw_mode_table[] = {
	GL_POINTS,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_LINES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_TRIANGLES
};

struct cx_gfx_mesh_gl_internals {
	GLuint vao;
	GLuint vbos[CX_GFX_MESH_MAX_ATTR_BUFFERS];
	GLuint vbos_len;
	GLuint ibo;
	GLenum ibo_type;
	GLenum draw_mode;
};

int    is_vertex_attribute_type_float(enum vertex_attribute_type vertex_attribute_type);
int    is_vertex_attribute_type_normalized(enum vertex_attribute_type vertex_attribute_type);

void cx_gfx_mesh_create(struct cx_gfx_mesh* p_mesh, const struct mesh_primitive* p_mesh_primitive) {
	struct cx_gfx_mesh_gl_internals* p_internals = (void*)p_mesh->_bytes;

    *p_mesh = (struct cx_gfx_mesh){0};

    glGenVertexArrays(1, &p_internals->vao);
    glBindVertexArray(p_internals->vao);

    p_internals->vbos_len = p_mesh_primitive->num_vertex_buffers;
    glGenBuffers(p_internals->vbos_len, p_internals->vbos);

    for (size_t i = 0; i < p_mesh_primitive->num_vertex_buffers; ++i) {
        const struct vertex_buffer* p_vertex_buffer = &p_mesh_primitive->p_vertex_buffers[i];

        glBindBuffer(GL_ARRAY_BUFFER, p_internals->vbos[i]);

        glBufferData(GL_ARRAY_BUFFER, p_vertex_buffer->size, p_vertex_buffer->p_bytes, GL_STATIC_DRAW);
    }

    for (size_t i = 0; i < p_mesh_primitive->num_attributes; ++i) {
        const struct vertex_attribute* p_attribute = &p_mesh_primitive->p_attributes[i];

        glBindBuffer(GL_ARRAY_BUFFER, p_internals->vbos[p_attribute->vertex_buffer_index]);

        if (is_vertex_attribute_type_float(p_attribute->layout.component_type)) {
            glVertexAttribPointer((GLuint)p_attribute->index,
                (GLint)p_attribute->layout.component_count,
                gl_vertex_attr_type_table[p_attribute->layout.component_type],
                (GLboolean)is_vertex_attribute_type_normalized(p_attribute->layout.component_type),
                (GLsizei)p_attribute->layout.stride,
                (void*)(GLsizeiptr)p_attribute->layout.offset
            );
        } else {
            glVertexAttribIPointer((GLuint)p_attribute->index,
                (GLint)p_attribute->layout.component_count,
                gl_vertex_attr_type_table[p_attribute->layout.component_type],
                (GLsizei)p_attribute->layout.stride,
                (void*)(GLsizeiptr)p_attribute->layout.offset
            );
        }
        
        glEnableVertexAttribArray(p_attribute->index);
    }

    if (p_mesh_primitive->index_buffer.p_bytes) {
        glGenBuffers(1, &p_internals->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p_internals->ibo);

        GLsizei index_size;
        switch(p_mesh_primitive->index_buffer.type) {
            case VERTEX_INDEX_TYPE_u8:  index_size = 1; break;
            case VERTEX_INDEX_TYPE_u16: index_size = 2; break;
            case VERTEX_INDEX_TYPE_u32: index_size = 4; break;
        }

        const GLsizei ibo_size = index_size * p_mesh_primitive->index_buffer.count;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ibo_size, p_mesh_primitive->index_buffer.p_bytes, GL_STATIC_DRAW);

        p_internals->ibo_type = gl_index_type_table[p_mesh_primitive->index_buffer.type];
        p_mesh->_elements_count = p_mesh_primitive->index_buffer.count;
    } else {
        p_mesh->_elements_count = p_mesh_primitive->vertex_count;
    }

	p_internals->draw_mode = gl_draw_mode_table[p_mesh_primitive->draw_mode];

    memcpy(p_mesh->_bounds_min, p_mesh_primitive->bounds_min, sizeof(p_mesh->_bounds_min));
    memcpy(p_mesh->_bounds_max, p_mesh_primitive->bounds_max, sizeof(p_mesh->_bounds_max));
}

void cx_gfx_mesh_destroy(struct cx_gfx_mesh* p_mesh) {
	struct cx_gfx_mesh_gl_internals* p_internals = (void*)p_mesh->_bytes;

    glDeleteBuffers(p_internals->vbos_len, p_internals->vbos);
    glDeleteBuffers(1, &p_internals->ibo);
    glDeleteVertexArrays(1, &p_internals->vao);
    *p_mesh = (struct cx_gfx_mesh){0};
}

void cx_gfx_mesh_draw(const struct cx_gfx_mesh* p_mesh) {
	const struct cx_gfx_mesh_gl_internals* p_internals = (const void*)p_mesh->_bytes;

    glBindVertexArray(p_internals->vao);

    if (p_internals->ibo) {
        glDrawElements(p_internals->draw_mode, p_mesh->_elements_count, p_internals->ibo_type, 0);
    } else {
        glDrawArrays(p_internals->draw_mode, 0, p_mesh->_elements_count);
    }
}

int is_vertex_attribute_type_float(enum vertex_attribute_type vertex_attribute_type) {
    return
        vertex_attribute_type == VERTEX_ATTRIBUTE_TYPE_f32 ||
        is_vertex_attribute_type_normalized(vertex_attribute_type);
}

int is_vertex_attribute_type_normalized(enum vertex_attribute_type vertex_attribute_type) {
    if (vertex_attribute_type >= VERTEX_ATTRIBUTE_TYPE_ni8 &&
        vertex_attribute_type <= VERTEX_ATTRIBUTE_TYPE_nu32) {
        return 1;
    }
    return 0;
}
