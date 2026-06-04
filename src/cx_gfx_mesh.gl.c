#include "cx_gfx_buffer.h"
#include "cx_gfx_buffer.gl.h"
#include "cx_gfx_mesh.h"
#include "cx_logging.h"
#include "cx_mesh_data.h"
#include "gl.h"
#include "vector.h"

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
	GL_NONE,
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
	GLenum ibo_type;
	GLenum draw_mode;
	struct cx_gfx_buffer vertex_buffers[CX_GFX_MESH_MAX_ATTR_BUFFERS];
	struct cx_gfx_buffer index_buffer;
};

void cx_gfx_mesh_create(
	const struct cx_mesh_data* p_mesh_data,
	enum cx_gfx_buffer_usage usage,
	struct cx_gfx_mesh* p_out) {

    *p_out = (struct cx_gfx_mesh){0};

	struct cx_gfx_mesh_gl_internals* p_internals = (void*)p_out->internals_.bytes_;

    glGenVertexArrays(1, &p_internals->vao);
    glBindVertexArray(p_internals->vao);

    for (size_t i = 0; i < p_mesh_data->layout.num_vertex_buffers; ++i) {
        const struct cx_mesh_vertex_buffer* p_vertex_buffer = &p_mesh_data->p_vertex_buffers[i];
		
		struct cx_gfx_buffer* p_vbuf = &p_internals->vertex_buffers[i];

		cx_gfx_buffer_create(
			CX_GFX_BUFFER_TYPE_vertex,
			p_vertex_buffer->size,
			usage,
			p_vbuf);

		cx_gfx_buffer_set(p_vbuf, p_vertex_buffer->size, p_vertex_buffer->p_bytes);
    }

    for (size_t i = 0; i < p_mesh_data->layout.num_attributes; ++i) {
        const struct cx_mesh_vertex_attribute* p_attribute = &p_mesh_data->layout.p_attributes[i];
		const struct cx_gfx_buffer* p_vbuf = &p_internals->vertex_buffers[p_attribute->vertex_buffer_index];
		const struct cx_gfx_buffer_gl_internals* p_vbuf_internals = (const void*)p_vbuf->internals_.bytes_;

        glBindBuffer(GL_ARRAY_BUFFER, p_vbuf_internals->id);

        if (is_vertex_attribute_type_float(p_attribute->format.type)) {
            glVertexAttribPointer((GLuint)p_attribute->index,
                (GLint)p_attribute->format.count,
                gl_vertex_attr_type_table[p_attribute->format.type],
                (GLboolean)is_vertex_attribute_type_normalized(p_attribute->format.type),
                (GLsizei)p_attribute->layout.stride,
                (void*)(GLsizeiptr)p_attribute->layout.offset
            );
        } else {
            glVertexAttribIPointer((GLuint)p_attribute->index,
                (GLint)p_attribute->format.count,
                gl_vertex_attr_type_table[p_attribute->format.type],
                (GLsizei)p_attribute->layout.stride,
                (void*)(GLsizeiptr)p_attribute->layout.offset
            );
        }
        
        glEnableVertexAttribArray((GLuint)p_attribute->index);
    }

    if (p_mesh_data->layout.index_type != CX_MESH_VERTEX_INDEX_TYPE_none) {
		const size_t index_buffer_size =
			cx_mesh_vertex_index_type_size(p_mesh_data->layout.index_type) * p_mesh_data->index_buffer.count;

		cx_gfx_buffer_create(CX_GFX_BUFFER_TYPE_index, index_buffer_size, usage, &p_internals->index_buffer);
		cx_gfx_buffer_set(&p_internals->index_buffer, index_buffer_size, p_mesh_data->index_buffer.p_bytes);

        p_internals->ibo_type = gl_index_type_table[p_mesh_data->layout.index_type];
        p_out->num_elements_ = p_mesh_data->index_buffer.count;
    } else {
        p_out->num_elements_ = p_mesh_data->vertex_count;
    }

	p_out->layout_hash_ = cx_mesh_data_layout_hash(&p_mesh_data->layout);

	p_internals->draw_mode = gl_draw_mode_table[p_mesh_data->layout.draw_mode];

	vec3_copy(p_mesh_data->bounds_min, p_out->aabb_min_);
	vec3_copy(p_mesh_data->bounds_max, p_out->aabb_max_);
}

void cx_gfx_mesh_destroy(struct cx_gfx_mesh* p_mesh) {
	struct cx_gfx_mesh_gl_internals* p_internals = (void*)p_mesh->internals_.bytes_;

	glDeleteVertexArrays(1, &p_internals->vao);

	*p_mesh = (struct cx_gfx_mesh) {0};
}

void cx_gfx_mesh_update(struct cx_gfx_mesh *p_mesh, const struct cx_mesh_data *p_mesh_data) {
	struct cx_gfx_mesh_gl_internals* p_internals = (void*)p_mesh->internals_.bytes_;

	if (p_mesh->layout_hash_ != cx_mesh_data_layout_hash(&p_mesh_data->layout)) {
		cx_gfx_mesh_destroy(p_mesh);
		cx_gfx_mesh_create(p_mesh_data, p_internals->vertex_buffers->usage_, p_mesh);
		return;
	}

	for (size_t i = 0; i < p_mesh_data->layout.num_vertex_buffers; ++i) {
        const struct cx_mesh_vertex_buffer* p_vertex_buffer = &p_mesh_data->p_vertex_buffers[i];
		
		struct cx_gfx_buffer* p_vbuf = &p_internals->vertex_buffers[i];
		
		cx_gfx_buffer_set(p_vbuf, p_vertex_buffer->size, p_vertex_buffer->p_bytes);
	}

	if (p_mesh_data->layout.index_type == CX_MESH_VERTEX_INDEX_TYPE_none) {
		const size_t index_buffer_size =
			cx_mesh_vertex_index_type_size(p_mesh_data->layout.index_type) * p_mesh_data->index_buffer.count;
		cx_gfx_buffer_set(&p_internals->index_buffer, index_buffer_size, p_mesh_data->index_buffer.p_bytes);
		p_mesh->num_elements_ = p_mesh_data->index_buffer.count;
	} else {
		p_mesh->num_elements_ = p_mesh_data->vertex_count;
	}

	vec3_copy(p_mesh_data->bounds_min, p_mesh->aabb_min_);
	vec3_copy(p_mesh_data->bounds_max, p_mesh->aabb_max_);
}

void cx_gfx_mesh_draw(const struct cx_gfx_mesh* p_mesh) {
	const struct cx_gfx_mesh_gl_internals* p_internals = (const void*)p_mesh->internals_.bytes_;

    glBindVertexArray(p_internals->vao);

    if (p_internals->index_buffer.size_ == 0) {
        glDrawArrays(p_internals->draw_mode, 0, (GLsizei)p_mesh->num_elements_);
    } else {
        glDrawElements(p_internals->draw_mode, (GLsizei)p_mesh->num_elements_, p_internals->ibo_type, 0);
    }
}
