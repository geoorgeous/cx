#include "darr.h"
#include "half_edge.h"
#include "stdlib.h"
#include "vector.h"

static int vec3_sort_cmp(const float* p_a, const float* p_b) {
	if (p_a[0] < p_b[0]) {
		return -1;
	}

	if (p_b[0] < p_a[0]) {
		return 1;
	}

	if (p_a[1] < p_b[1]) {
		return -1;
	}

	if (p_b[1] < p_a[1]) {
		return 1;
	}

	if (p_a[2] < p_b[2]) {
		return -1;
	}

	if (p_b[2] < p_a[2]) {
		return 1;
	}

	return 0;
}

void half_edge_get_vertices(const struct he_mesh* p_mesh, float* p_vertices, size_t* p_num_vertices) {
    struct darr vertices;
    darr_init(&vertices, sizeof(float) * 3);

    struct he_face* p_face = p_mesh->p_faces;
    while (p_face) {
        struct he_edge* p_edge = p_face->p_edges;
        do {
            float* p_v = darr_push(&vertices);
            vec3_set(p_edge->p_tail->position, p_v);
            p_edge = p_edge->p_next;
        } while (p_edge != p_face->p_edges);
        p_face = p_face->p_next;
    }

    if (vertices._length == 0) {
        *p_num_vertices = 0;
        return;
    }

    qsort(vertices._p_buffer, vertices._length, vertices._element_size, (void*)vec3_sort_cmp);

    size_t m = 1;

    if (p_vertices) {
        vec3_set(darr_get(&vertices, 0), &p_vertices[0]);
        for (size_t i = 1; i < vertices._length; ++i) {
            float* p_a = darr_get(&vertices, i);
            float* p_b = darr_get(&vertices, m - 1);
            if (!vec3_cmp(p_a, p_b)) {
                if (i != m) {
                    vec3_set(p_a, darr_get(&vertices, m));
                    vec3_set(p_a, &p_vertices[m * 3]);
                }
                ++m;
            }
        }
    } else {
        for (size_t i = 1; i < vertices._length; ++i) {
            const float* p_a = darr_get(&vertices, i);
            const float* p_b = darr_get(&vertices, m - 1);
            if (!vec3_cmp(p_a, p_b)) {
                if (i != m) {
                    vec3_set(p_a, darr_get(&vertices, m));
                }
                ++m;
            }
        }
    }

    darr_free(&vertices);

    *p_num_vertices = m;
}