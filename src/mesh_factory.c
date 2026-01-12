#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "darr.h"
#include "half_edge.h"
#include "mesh_factory.h"
#include "mesh.h"

void mesh_factory_make_quad(const float* p_half_size, struct mesh_primitive* p_out_mesh_primitive) {
    const float vertices[] = {
        -p_half_size[0], 0, -p_half_size[1], 0, 1, 0,
         p_half_size[0], 0,  p_half_size[1], 0, 1, 0,
         p_half_size[0], 0, -p_half_size[1], 0, 1, 0,

         p_half_size[0], 0,  p_half_size[1], 0, 1, 0,
        -p_half_size[0], 0, -p_half_size[1], 0, 1, 0,
        -p_half_size[0], 0,  p_half_size[1], 0, 1, 0
    };
    
    const size_t num_vertices = 6;
    const size_t vertex_size = sizeof(float) * 6;
    const size_t vertices_size = num_vertices * vertex_size;

    float* p_vertices = malloc(vertices_size);
    memcpy(p_vertices, vertices, vertices_size);

    *p_out_mesh_primitive = (struct mesh_primitive) {
        .p_vertex_buffers = malloc(sizeof(*p_out_mesh_primitive->p_vertex_buffers)),
        .num_vertex_buffers = 1,
        .p_attributes = malloc(sizeof(*p_out_mesh_primitive->p_attributes) * 2),
        .num_attributes = 2,
        .vertex_count = num_vertices,
        .draw_mode = MESH_PRIMITIVE_DRAW_MODE_triangles,
        .bounds_min = { -p_half_size[0], 0, -p_half_size[1] },
        .bounds_max = {  p_half_size[0],  0, p_half_size[1] }
    };

    *p_out_mesh_primitive->p_vertex_buffers = (struct vertex_buffer) {
        .p_bytes = p_vertices,
        .size = vertices_size
    };

    p_out_mesh_primitive->p_attributes[0] = (struct vertex_attribute) {
        .index = 0,
        .vertex_buffer_index = 0,
        .layout = {
            .stride = vertex_size,
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };

    p_out_mesh_primitive->p_attributes[1] = (struct vertex_attribute) {
        .index = 1,
        .vertex_buffer_index = 0,
        .layout = {
            .offset = sizeof(float) * 3,
            .stride = vertex_size,
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };
}

void mesh_factory_make_box(const float* p_half_size, struct mesh_primitive* p_out_mesh_primitive) {
    const float vertices[] = {
        -p_half_size[0], -p_half_size[1], -p_half_size[2], -1,  0,  0,
        -p_half_size[0],  p_half_size[1],  p_half_size[2], -1,  0,  0,
        -p_half_size[0],  p_half_size[1], -p_half_size[2], -1,  0,  0,

        -p_half_size[0],  p_half_size[1],  p_half_size[2], -1,  0,  0,
        -p_half_size[0], -p_half_size[1], -p_half_size[2], -1,  0,  0,
        -p_half_size[0], -p_half_size[1],  p_half_size[2], -1,  0,  0,

         p_half_size[0], -p_half_size[1],  p_half_size[2],  1,  0,  0,
         p_half_size[0],  p_half_size[1], -p_half_size[2],  1,  0,  0,
         p_half_size[0],  p_half_size[1],  p_half_size[2],  1,  0,  0,

         p_half_size[0],  p_half_size[1], -p_half_size[2],  1,  0,  0,
         p_half_size[0], -p_half_size[1],  p_half_size[2],  1,  0,  0,
         p_half_size[0], -p_half_size[1], -p_half_size[2],  1,  0,  0,

        -p_half_size[0], -p_half_size[1], -p_half_size[2],  0, -1,  0,
         p_half_size[0], -p_half_size[1],  p_half_size[2],  0, -1,  0,
        -p_half_size[0], -p_half_size[1],  p_half_size[2],  0, -1,  0,

         p_half_size[0], -p_half_size[1],  p_half_size[2],  0, -1,  0,
        -p_half_size[0], -p_half_size[1], -p_half_size[2],  0, -1,  0,
         p_half_size[0], -p_half_size[1], -p_half_size[2],  0, -1,  0,

         p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  1,  0,
        -p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  1,  0,
         p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  1,  0,

        -p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  1,  0,
         p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  1,  0,
        -p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  1,  0,

        -p_half_size[0], -p_half_size[1], -p_half_size[2],  0,  0, -1,
         p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  0, -1,
         p_half_size[0], -p_half_size[1], -p_half_size[2],  0,  0, -1,

         p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  0, -1,
        -p_half_size[0], -p_half_size[1], -p_half_size[2],  0,  0, -1,
        -p_half_size[0],  p_half_size[1], -p_half_size[2],  0,  0, -1,

        -p_half_size[0], -p_half_size[1],  p_half_size[2],  0,  0,  1,
         p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  0,  1,
        -p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  0,  1,

         p_half_size[0],  p_half_size[1],  p_half_size[2],  0,  0,  1,
        -p_half_size[0], -p_half_size[1],  p_half_size[2],  0,  0,  1,
         p_half_size[0], -p_half_size[1],  p_half_size[2],  0,  0,  1,
    };
    
    const size_t num_vertices = 36;
    const size_t vertex_size = sizeof(float) * 6;
    const size_t vertices_size = num_vertices * vertex_size;

    float* p_vertices = malloc(vertices_size);
    memcpy(p_vertices, vertices, vertices_size);

    *p_out_mesh_primitive = (struct mesh_primitive) {
        .p_vertex_buffers = malloc(sizeof(*p_out_mesh_primitive->p_vertex_buffers)),
        .num_vertex_buffers = 1,
        .p_attributes = malloc(sizeof(*p_out_mesh_primitive->p_attributes) * 2),
        .num_attributes = 2,
        .vertex_count = num_vertices,
        .draw_mode = MESH_PRIMITIVE_DRAW_MODE_triangles,
        .bounds_min = { -p_half_size[0], -p_half_size[1], -p_half_size[2] },
        .bounds_max = {  p_half_size[0],  p_half_size[1],  p_half_size[2] }
    };

    *p_out_mesh_primitive->p_vertex_buffers = (struct vertex_buffer) {
        .p_bytes = p_vertices,
        .size = vertices_size
    };

    p_out_mesh_primitive->p_attributes[0] = (struct vertex_attribute) {
        .index = 0,
        .vertex_buffer_index = 0,
        .layout = {
            .stride = vertex_size,
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };

    p_out_mesh_primitive->p_attributes[1] = (struct vertex_attribute) {
        .index = 1,
        .vertex_buffer_index = 0,
        .layout = {
            .offset = sizeof(float) * 3,
            .stride = vertex_size,
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };
}

void mesh_factory_make_sphere(float radius, unsigned int n, struct mesh_primitive* p_out_mesh_primitive) {
    const size_t num_vertices = (n - 1) * n + 2;
    const size_t vertex_size = sizeof(float) * 6;
    const size_t vertices_size = num_vertices * vertex_size;
    float* p_vertices = malloc(vertices_size);

    float* p_v = p_vertices;
    *p_v++ = 0;
    *p_v++ = radius;
    *p_v++ = 0;

    *p_v++ = 0;
    *p_v++ = 1;
    *p_v++ = 0;

    const float s = 6.283185 / n;
    const float t = s * 0.5f;

    for (size_t i = 1; i < n; ++i) {
        const float it = i * t;
        const float cit = cosf(it);
        const float sit = sinf(it);
        for (size_t j = 0; j < n; ++j) {
            const float js = j * s;
            const float cjs = cosf(js);
            const float sjs = sinf(js);
            
            const float x = cjs * sit;
            const float y = cit;
            const float z = sjs * sit;

            *p_v++ = radius * x;
            *p_v++ = radius * y;
            *p_v++ = radius * z;
    
            *p_v++ = x;
            *p_v++ = y;
            *p_v++ = z;
        }
    }

    *p_v++ = 0;
    *p_v++ = -radius;
    *p_v++ = 0;

    *p_v++ = 0;
    *p_v++ = -1;
    *p_v++ = 0;

    const size_t num_indices = (n * n - n) * 6;
    unsigned short* p_indices = malloc(num_indices * sizeof(unsigned short));

    unsigned short* p_i = p_indices;

    // todo: fix ends of rows of faces

    // north pole faces
    for (size_t i = 0; i < n; ++i) {
        *p_i++ = 0;
        *p_i++ = i + 2;
        *p_i++ = i + 1;
    }

    // interpole rings
    for (size_t i = 0; i < n - 2; ++i) {
        for (size_t j = 0; j < n; ++j) {
            *p_i++ = i * n + j + 2;
            *p_i++ = i * n + j + 1 + n;
            *p_i++ = i * n + j + 1;
            
            *p_i++ = i * n + j + 2;
            *p_i++ = i * n + j + 2 + n;
            *p_i++ = i * n + j + 1 + n;
        }
    }
    
    // south pole
    for (size_t i = 0; i < n; ++i) {
        *p_i++ = num_vertices - 1;
        *p_i++ = num_vertices - 1 - (i + 2);
        *p_i++ = num_vertices - 1 - (i + 1);
    }

    *p_out_mesh_primitive = (struct mesh_primitive) {
        .p_vertex_buffers = malloc(sizeof(*p_out_mesh_primitive->p_vertex_buffers)),
        .num_vertex_buffers = 1,
        .p_attributes = malloc(sizeof(*p_out_mesh_primitive->p_attributes) * 2),
        .num_attributes = 2,
        .vertex_count = num_vertices,
        .index_buffer = {
            .p_bytes = p_indices,
            .count = num_indices,
            .type = VERTEX_INDEX_TYPE_u16
        },
        .draw_mode = MESH_PRIMITIVE_DRAW_MODE_triangles,
        .bounds_min = { -radius, -radius, -radius },
        .bounds_max = {  radius,  radius,  radius }
    };

    *p_out_mesh_primitive->p_vertex_buffers = (struct vertex_buffer) {
        .p_bytes = p_vertices,
        .size = vertices_size
    };

    p_out_mesh_primitive->p_attributes[0] = (struct vertex_attribute) {
        .index = 0,
        .vertex_buffer_index = 0,
        .layout = {
            .stride = vertex_size,
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };

    p_out_mesh_primitive->p_attributes[1] = (struct vertex_attribute) {
        .index = 1,
        .vertex_buffer_index = 0,
        .layout = {
            .offset = sizeof(float) * 3,
            .stride = vertex_size,
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };
}

void mesh_factory_make_hemisphere(float radius, unsigned int n, struct mesh_primitive* p_out_mesh_primitive) {
	mesh_factory_make_sphere(radius, n, p_out_mesh_primitive);
}

void mesh_factory_make_cylinder(
	float radius_a, float radius_b, float half_length,
	unsigned int n,
	int b_cap_a, int b_cap_b,
	struct mesh_primitive* p_out_mesh_primitive) {
	
	// todo:
	(void)radius_b;
	(void)half_length;
	(void)b_cap_a;
	(void)b_cap_b;

	mesh_factory_make_sphere(radius_a, n, p_out_mesh_primitive);
}

void mesh_factory_make_from_halfedge_mesh(
	const struct he_mesh* p_he_mesh,
	struct mesh_primitive* p_out_mesh_primitive) {
    
	struct darr vertices;
    darr_init(&vertices, sizeof(float) * 3);

    struct he_face* p_face = p_he_mesh->p_faces;

	while (p_face) {
		size_t num_vertices = 0;

		struct he_edge* p_edge = p_face->p_edges;
		do {
			float* p_vertex = darr_push(&vertices);
			p_vertex[0] = p_edge->p_tail->position[0];
			p_vertex[1] = p_edge->p_tail->position[1];
			p_vertex[2] = p_edge->p_tail->position[2];

			++num_vertices;

			if (num_vertices > 3) {
				p_vertex = darr_push(&vertices);
				p_vertex[0] = p_face->p_edges->p_tail->position[0];
				p_vertex[1] = p_face->p_edges->p_tail->position[1];
				p_vertex[2] = p_face->p_edges->p_tail->position[2];
				
				p_vertex = darr_push(&vertices);
				p_vertex[0] = p_edge->p_prev->p_tail->position[0];
				p_vertex[1] = p_edge->p_prev->p_tail->position[1];
				p_vertex[2] = p_edge->p_prev->p_tail->position[2];
			}

			p_edge = p_edge->p_next;
		} while (p_edge != p_face->p_edges);

		p_face = p_face->p_next;
	};

    darr_shrink(&vertices);

    *p_out_mesh_primitive = (struct mesh_primitive) {
        .p_vertex_buffers = malloc(sizeof(*p_out_mesh_primitive->p_vertex_buffers)),
        .num_vertex_buffers = 1,
        .p_attributes = malloc(sizeof(*p_out_mesh_primitive->p_attributes) * 1),
        .num_attributes = 1,
        .vertex_count = vertices._length,
        .draw_mode = MESH_PRIMITIVE_DRAW_MODE_lines
    };

    *p_out_mesh_primitive->p_vertex_buffers = (struct vertex_buffer) {
        .p_bytes = vertices._p_buffer,
        .size = vertices._capacity * vertices._element_size
    };

    p_out_mesh_primitive->p_attributes[0] = (struct vertex_attribute) {
        .index = 0,
        .vertex_buffer_index = 0,
        .layout = {
            .stride = vertices._element_size,
            .component_count = 3,
            .component_type = VERTEX_ATTRIBUTE_TYPE_f32
        }
    };
}

void mesh_factory_free_primitive(struct mesh_primitive* p_out_mesh_primitive) {
    free(p_out_mesh_primitive->p_vertex_buffers[0].p_bytes);
    free(p_out_mesh_primitive->p_vertex_buffers);
    free(p_out_mesh_primitive->p_attributes);
    free(p_out_mesh_primitive->index_buffer.p_bytes);
    *p_out_mesh_primitive = (struct mesh_primitive) {0};
}
