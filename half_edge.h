#ifndef _H__HALF_EDGE
#define _H__HALF_EDGE

#include <stdint.h>

struct he_vertex {
	struct he_vertex* p_prev;
	struct he_vertex* p_next;
	float position[3];
};

struct he_edge {
	struct he_edge*   p_prev;
	struct he_edge*   p_next;
	struct he_edge*   p_twin;
	struct he_vertex* p_tail;
	struct he_face*   p_face;

	struct he_edge*   p_qh_horizon_next;
};

struct he_face {
	struct he_face* p_prev;
	struct he_face* p_next;
	struct he_edge* p_edges;

	struct he_vertex* p_qh_conflict_list;
	int               b_qh_processed;
}; 

struct he_mesh {
	void*             p_buffer;
	struct he_vertex* p_free_vertices;
	struct he_edge*   p_free_edges;
	struct he_face*   p_free_faces;
	struct he_face*   p_faces;
};

void half_edge_get_vertices(const struct he_mesh* p_mesh, float* p_vertices, size_t* p_num_vertices);

#endif