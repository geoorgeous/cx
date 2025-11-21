#include <float.h>
#include <malloc.h>

#include "logging.h"
#include "math_utils.h"
#include "mesh.h"
#include "quickhull.h"
#include "static_mesh.h"
#include "vector.h"

#define QH_EPSILON_SCALE 1000

#define CX_LOG_CAT_QH "quickhull"

#define QH_DEBUG_LOG_EDGE(P_EDGE) CX_DBG_LOG_FMT(QH_LOG_CAT, "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p, p_twin=%p\n",\
				P_EDGE, P_EDGE->p_tail->position[0], P_EDGE->p_tail->position[1], P_EDGE->p_tail->position[2],\
				P_EDGE->p_next->p_tail->position[0], P_EDGE->p_next->p_tail->position[1], P_EDGE->p_next->p_tail->position[2],\
				P_EDGE->p_face, P_EDGE->p_prev, P_EDGE->p_next, P_EDGE->p_twin);\

#define QH_DEBUG_LOG_FACE(P_FACE) {\
		CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\tFACE (%p) - Processed: %d, p_prev=%p, p_next=%p\n", P_FACE, P_FACE->b_qh_processed, P_FACE->p_prev, P_FACE->p_next);\
		struct he_edge* p_edge_debug = P_FACE->p_edges;\
		do {\
			QH_DEBUG_LOG_EDGE(p_edge_debug);\
			p_edge_debug = p_edge_debug->p_next;\
		} while(p_edge_debug != P_FACE->p_edges);\
		struct he_vertex* p_vertex_debug = P_FACE->p_qh_conflict_list;\
		while (p_vertex_debug) {\
			CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\t\tConflict Vertex (%p) [%f, %f, %f], p_prev=%p, p_next=%p\n",\
				p_vertex_debug, p_vertex_debug->position[0], p_vertex_debug->position[1], p_vertex_debug->position[2], p_vertex_debug->p_prev, p_vertex_debug->p_next);\
			p_vertex_debug = p_vertex_debug->p_next;\
		} }

#define QH_DEBUG_LOG_HULL(P_HULL, NOTE) {\
		CX_DBG_LOG(CX_LOG_CAT_QH, "HULL FACES ("NOTE"):\n");\
		struct he_face* p_face_debug = P_HULL->p_faces;\
		while (p_face_debug) {\
			QH_DEBUG_LOG_FACE(p_face_debug);\
			p_face_debug = p_face_debug->p_next;\
		} }\


#define QH_DEBUG 0

struct qh_freelist_node {
	struct qh_freelist_node* p_prev;
	struct qh_freelist_node* p_next;
};

static void                     qh_freelist_init(struct qh_freelist_node* p_head, size_t node_size, size_t n);
static struct qh_freelist_node* qh_freelist_get(struct qh_freelist_node** pp_list);
static void                     qh_freelist_set_head(struct qh_freelist_node** pp_list, struct qh_freelist_node* p_node);
static void                     qh_freelist_erase(struct qh_freelist_node** pp_list, struct qh_freelist_node* p_node);
static void                     qh_freelist_append(struct qh_freelist_node** pp_list, struct qh_freelist_node* p_node);

static void  qh_compute_triangle_normal(const float* p_a, const float* p_b, const float* p_c, float* p_result);
static float qh_face_vertex_dist_sq(const struct he_face* p_face, const struct he_vertex* p_vertex);
static int   qh_is_point_above_abc_plane(const float* p_a, const float* p_b, const float* p_c, const float* p_point, float threshold);
static int   qh_is_point_above_face_plane(const struct he_face* p_face, const float* p_point, float threshold);
static void  qh_compute_face_center(const struct he_face* p_face, float* p_result);
static int   qh_edge_is_convex(const struct he_edge* p_edge, float threshold);

static void qh_purge_duplicate_input_points(float* p_point_cloud, size_t* p_num_points);
static void qh_find_initial_hull_vertices(const float* p_point_cloud, size_t num_points, struct he_mesh* p_hull, struct he_vertex** pp_vertices, float* p_threshold);
static void qh_find_initial_hull_half_edge_pairs(struct he_mesh* p_hull);

static struct he_face* qh_construct_new_face(struct he_mesh* p_hull, struct he_vertex* p_fa, struct he_vertex* p_fb, struct he_vertex* p_fc);
static void            qh_merge_edge_faces(struct he_mesh* p_hull, struct he_edge* p_edge);
static void            qh_join_edges(struct he_edge* p_edge_head, struct he_edge* p_edge_tail);
static void            qh_twin_edges(struct he_edge* p_edge_a, struct he_edge* p_edge_b);
static void            qh_delete_hull_face(struct he_mesh* p_hull, struct he_face* p_face);

static struct he_vertex* qh_next_conflict_vertex(struct he_mesh* p_hull, struct he_face** pp_face);
static void              qh_find_horizon(struct he_edge* p_edge, const struct he_vertex* p_eye, struct he_edge** pp_horizon_start);
static void              qh_construct_horizon_faces(struct he_mesh* p_hull, struct he_edge* p_horizon_start, struct he_vertex* p_eye, float threshold);
static void              qh_distribute_conflict_vertices(struct he_mesh* p_hull, struct he_face* p_faces, struct he_vertex* p_vertices, float threshold);
static void              qh_merge_coplanars(struct he_mesh* p_hull, float threshold);
static void              qh_fix_topology_errors(struct he_mesh* p_hull, struct he_edge* p_edge_i, struct he_edge* p_edge_o);
static void              qh_rebuild_face_plane(struct he_face* p_face);

void qh_freelist_init(struct qh_freelist_node* p_head, size_t node_size, size_t n) {
	struct qh_freelist_node* p_node = p_head;

	p_node->p_prev = 0;

	for (size_t i = 0; i < n - 1; ++i) {
		p_node->p_next = (struct qh_freelist_node*)((char*)p_node + node_size);
		p_node->p_next->p_prev = p_node;
		p_node = p_node->p_next;
	}

	p_node->p_next = 0;
}

struct qh_freelist_node* qh_freelist_get(struct qh_freelist_node** pp_list) {
	if (!*pp_list) {
		cx_log(CX_LOG_ERROR, CX_LOG_CAT_QH, "Freelist empty!\n");
		return 0;
	}

	struct qh_freelist_node* r = *pp_list;
	
	*pp_list = r->p_next;
	if (*pp_list) {
		(*pp_list)->p_prev = 0;
	}
	
	r->p_next = 0;
	return r;
}

void qh_freelist_set_head(struct qh_freelist_node** pp_list, struct qh_freelist_node* p_node) {
	p_node->p_next = *pp_list;
	p_node->p_prev = 0;

	if (p_node->p_next) {
		p_node->p_next->p_prev = p_node;
	}
	
	*pp_list = p_node;
}

void qh_freelist_erase(struct qh_freelist_node** pp_list, struct qh_freelist_node* p_node) {
	if (p_node->p_prev) {
		p_node->p_prev->p_next = p_node->p_next;
	} else {
		*pp_list = p_node->p_next;
	}
	
	if (p_node->p_next) {
		p_node->p_next->p_prev = p_node->p_prev;
	}

	p_node->p_prev =
	p_node->p_next = 0;
}

void qh_freelist_append(struct qh_freelist_node** pp_list, struct qh_freelist_node* p_node) {
	if (!p_node) {
		return;
	}

	if (*pp_list) {
		struct qh_freelist_node* p_n = *pp_list;
		while (p_n->p_next) {
			p_n = p_n->p_next;
		}
		p_n->p_next = p_node;
		p_node->p_prev = p_n;
	} else {
		*pp_list = p_node;
	}
}

void qh_compute_triangle_normal(const float* p_a, const float* p_b, const float* p_c, float* p_result) {
	float tmp[3];

	vec3_sub(p_b, p_a, tmp);
	vec3_sub(p_c, p_a, p_result);

	vec3_cross(tmp, p_result, p_result);
	vec3_norm(p_result, p_result);
}

float qh_face_vertex_dist_sq(const struct he_face* p_face, const struct he_vertex* p_vertex) {
	// todo: we might be able to get away with not normalizing the normal here since we only want dist_sq

	float normal[3];
	qh_compute_triangle_normal(
		p_face->p_edges->p_tail->position,
		p_face->p_edges->p_next->p_tail->position,
		p_face->p_edges->p_next->p_next->p_tail->position,
		normal);

	struct he_edge* p_edge = p_face->p_edges;
	do {	
		float av[3];
		vec3_sub(p_vertex->position, p_edge->p_tail->position, av);

		float edge_segment[3];
		vec3_sub(p_edge->p_next->p_tail->position, p_edge->p_tail->position, edge_segment);

		float cross[3];
		vec3_cross(edge_segment, av, cross);

		if (vec3_dot(cross, normal) < 0) {
			// Vertex projection lays outside polygon. Compute closest edge distance.

			float min_edge_dist_sq = FLT_MAX;

			p_edge = p_face->p_edges;
			do {
				const float edge_dist_sq = segment_point_dist_sq(p_edge->p_tail->position, p_edge->p_next->p_tail->position, p_vertex->position);
				if (edge_dist_sq < min_edge_dist_sq) {
					min_edge_dist_sq = edge_dist_sq;
				}
				p_edge = p_edge->p_next;
			} while(p_edge != p_face->p_edges);

			return min_edge_dist_sq;
		}

		p_edge = p_edge->p_next;
	} while(p_edge != p_face->p_edges);

	// Vertex projection lays inside polygon bounds. Return vertex distance to polygon plane.

	float point_to_plane[3];
	vec3_sub(p_vertex->position, p_face->p_edges->p_tail->position, point_to_plane);

	const float divisor = vec3_dot(normal, normal);
	const float d = vec3_dot(point_to_plane, normal) / divisor;

	return d * d;
}

void qh_find_initial_hull_vertices(const float* p_point_cloud, size_t num_points, struct he_mesh* p_hull, struct he_vertex** pp_vertices, float* p_threshold) {
	// Find the six extreme points of the point cloud (-X, +X, -Y, +Y, -Z, +Z)

	size_t extremes[6] = {0};

	for (size_t i = 0; i < num_points; ++i) {
		const float* p = &p_point_cloud[i * 3];
		vec3_set(p, p_hull->p_free_vertices[i].position);
		if (p[0] < p_point_cloud[extremes[0] * 3 + 0]) {
			extremes[0] = i;
		}
		if (p[0] > p_point_cloud[extremes[1] * 3 + 0]) {
			extremes[1] = i;
		}
		if (p[1] < p_point_cloud[extremes[2] * 3 + 1]) {
			extremes[2] = i;
		}
		if (p[1] > p_point_cloud[extremes[3] * 3 + 1]) {
			extremes[3] = i;
		}
		if (p[2] < p_point_cloud[extremes[4] * 3 + 2]) {
			extremes[4] = i;
		}
		if (p[2] > p_point_cloud[extremes[5] * 3 + 2]) {
			extremes[5] = i;
		}
	}

	*p_threshold = FLT_EPSILON * QH_EPSILON_SCALE * 3 * (
		fmaxf(fabsf(p_point_cloud[extremes[0] * 3 + 0]), fabsf(p_point_cloud[extremes[1] * 3 + 0])) +
		fmaxf(fabsf(p_point_cloud[extremes[2] * 3 + 1]), fabsf(p_point_cloud[extremes[3] * 3 + 1])) +
		fmaxf(fabsf(p_point_cloud[extremes[4] * 3 + 2]), fabsf(p_point_cloud[extremes[1] * 5 + 2])));

	// CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Point cloud extremes:\n\tx_axis=([%f, %f, %f], [%f, %f, %f])\n\ty_axis=([%f, %f, %f], [%f, %f, %f])\n\tz_axis=([%f, %f, %f], [%f, %f, %f])\n"
	// 	, p_point_cloud[extremes[0] * 3 + 0], p_point_cloud[extremes[0] * 3 + 1], p_point_cloud[extremes[0] * 3 + 2], p_point_cloud[extremes[1] * 3 + 0], p_point_cloud[extremes[1] * 3 + 1], p_point_cloud[extremes[1] * 3 + 2]
	// 	, p_point_cloud[extremes[2] * 3 + 0], p_point_cloud[extremes[2] * 3 + 1], p_point_cloud[extremes[2] * 3 + 2], p_point_cloud[extremes[3] * 3 + 0], p_point_cloud[extremes[3] * 3 + 1], p_point_cloud[extremes[3] * 3 + 2]
	// 	, p_point_cloud[extremes[4] * 3 + 0], p_point_cloud[extremes[4] * 3 + 1], p_point_cloud[extremes[4] * 3 + 2], p_point_cloud[extremes[5] * 3 + 0], p_point_cloud[extremes[5] * 3 + 1], p_point_cloud[extremes[5] * 3 + 2]);

	// Pick the two opposing vertices that are farthest apart to be the first two points of our tetrahedron

	size_t iv[4];

	float max;

	max = -FLT_MAX;
	for (size_t i = 0; i < 6; ++i) {
		for (size_t j = 0; j < 6; ++j) {
			const float d = vec3_dist_sq(&p_point_cloud[extremes[i] * 3], &p_point_cloud[extremes[j] * 3]);
			if (d > max) {
				max = d;
				iv[0] = extremes[i];
				iv[1] = extremes[j];
			}
		}
	}

	// Of the remaining vertices, pick the one farthest away from the line to form a triangle

	max = -FLT_MAX;
	for (size_t i = 0; i < num_points; ++i) {
		if (i == iv[0] || i == iv[1]) {
			continue;
		}
		const float* p = &p_point_cloud[i * 3];
		const float  d = segment_point_dist_sq(&p_point_cloud[iv[0] * 3], &p_point_cloud[iv[1] * 3], p);
		if (d > max) {
			max = d;
			iv[2] = i;
		}
		// CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Segment: [%f, %f, %f]->[%f, %f, %f], Point: [%f, %f, %f], distance: %f\n"
		// 	, p_point_cloud[iv[0] * 3], p_point_cloud[iv[0] * 3 + 1], p_point_cloud[iv[0] * 3 + 2]
		// 	, p_point_cloud[iv[1] * 3], p_point_cloud[iv[1] * 3 + 1], p_point_cloud[iv[1] * 3 + 2]
		// 	, p[0], p[1], p[2]
		// 	, sqrtf(d));
	}

	max = -FLT_MAX;
	for (size_t i = 0; i < num_points; ++i) {
		if (i == iv[0] || i == iv[1] || i == iv[2]) {
			continue;
		}

		const float* p = &p_point_cloud[i * 3];

		float normal[3];

		float tmp[3];

		vec3_sub(&p_point_cloud[iv[1] * 3], &p_point_cloud[iv[0] * 3], tmp);
		vec3_sub(&p_point_cloud[iv[2] * 3], &p_point_cloud[iv[0] * 3], normal);
		vec3_cross(tmp, normal, normal);
		vec3_norm(normal, normal);

		float point_to_plane[3];
		vec3_sub(p, &p_point_cloud[iv[0] * 3], point_to_plane);

		const float divisor = vec3_dot(normal, normal);
		const float d = vec3_dot(point_to_plane, normal) / divisor;
		const float d_sq = d * d;
		if (d_sq > max) {
			max = d_sq;
			iv[3] = i;  
		}
	}

	pp_vertices[0] = &p_hull->p_free_vertices[iv[0]];
	pp_vertices[1] = &p_hull->p_free_vertices[iv[1]];
	pp_vertices[2] = &p_hull->p_free_vertices[iv[2]];
	pp_vertices[3] = &p_hull->p_free_vertices[iv[3]];

	for (size_t i = 0; i < 4; ++i) {
		if (pp_vertices[i]->p_prev) {
			pp_vertices[i]->p_prev->p_next = pp_vertices[i]->p_next;
		} else {
			p_hull->p_free_vertices = pp_vertices[i]->p_next;
		}
		
		if (pp_vertices[i]->p_next) {
			pp_vertices[i]->p_next->p_prev = pp_vertices[i]->p_prev;
		}

		pp_vertices[i]->p_prev =
		pp_vertices[i]->p_next = 0;
	}
}

void qh_join_edges(struct he_edge* p_edge_head, struct he_edge* p_edge_tail) {
	p_edge_head->p_prev = p_edge_tail;
	p_edge_tail->p_next = p_edge_head;
}

struct he_face* qh_construct_new_face(struct he_mesh* p_hull, struct he_vertex* p_fa, struct he_vertex* p_fb, struct he_vertex* p_fc) {
	// CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "New face: [%f, %f, %f]->[%f, %f, %f]->[%f, %f, %f]\n"
	// 	, p_fa->position[0], p_fa->position[1], p_fa->position[2]
	// 	, p_fb->position[0], p_fb->position[1], p_fb->position[2]
	// 	, p_fc->position[0], p_fc->position[1], p_fc->position[2]);

	struct he_face* p_face = (struct he_face*)qh_freelist_get((struct qh_freelist_node**)&p_hull->p_free_faces);
	*p_face = (struct he_face){0};

	qh_freelist_set_head((struct qh_freelist_node**)&p_face->p_edges, qh_freelist_get((struct qh_freelist_node**)&p_hull->p_free_edges));
	qh_freelist_set_head((struct qh_freelist_node**)&p_face->p_edges, qh_freelist_get((struct qh_freelist_node**)&p_hull->p_free_edges));
	qh_freelist_set_head((struct qh_freelist_node**)&p_face->p_edges, qh_freelist_get((struct qh_freelist_node**)&p_hull->p_free_edges));

	qh_join_edges(p_face->p_edges, p_face->p_edges->p_next->p_next);

	p_face->p_edges->p_tail = p_fa;
	p_face->p_edges->p_next->p_tail = p_fb;
	p_face->p_edges->p_next->p_next->p_tail = p_fc;

	struct he_edge* p_edge = p_face->p_edges;
	do {
		p_edge->p_face = p_face;
		p_edge->p_twin =
		p_edge->p_qh_horizon_next = 0;

		p_edge = p_edge->p_next;
	} while(p_edge != p_face->p_edges);
	
	return p_face;
}

void qh_find_initial_hull_half_edge_pairs(struct he_mesh* p_hull) {
	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		struct he_edge* p_edge = p_face->p_edges;
		do {
			struct he_face* p_face_other = p_hull->p_faces;
			while (!p_edge->p_twin && p_face_other) {
				if (p_face_other != p_face) {
					struct he_edge* p_edge_other = p_face_other->p_edges;

					if (p_edge_other == p_edge) {
						break;
					}

					do {
						if (!p_edge_other->p_twin && p_edge->p_tail == p_edge_other->p_next->p_tail && p_edge->p_next->p_tail == p_edge_other->p_tail) {
							p_edge->p_twin = p_edge_other;
							p_edge_other->p_twin = p_edge;
							break;
						}

						p_edge_other = p_edge_other->p_next;
					} while(p_edge_other != p_face_other->p_edges);
				}

				p_face_other = p_face_other->p_next;
			}

			p_edge = p_edge->p_next;
		} while(p_edge != p_face->p_edges);

		p_face = p_face->p_next;
	}
}

void qh_distribute_conflict_vertices(struct he_mesh* p_hull, struct he_face* p_faces, struct he_vertex* p_vertices, float threshold) {
	struct he_vertex* p_vertex = p_vertices;
	while (p_vertex) {
		struct he_face* p_closest_face = 0;
		float closest_face_dist_sq = FLT_MAX;

		struct he_face* p_face = p_faces;
		while (p_face) {
			if (qh_is_point_above_face_plane(p_face, p_vertex->position, -threshold)) {
				const float face_dist_sq = qh_face_vertex_dist_sq(p_face, p_vertex);

				if (face_dist_sq < closest_face_dist_sq) {
					closest_face_dist_sq = face_dist_sq;
					p_closest_face = p_face;
				}
			}

			p_face = p_face->p_next;
		}

#if QH_DEBUG
		if (p_closest_face) {
			CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\tConflict vertex (%p) moved to face (%p) conflict list\n", p_vertex, p_closest_face);
		} else {
			CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\tConflict vertex (%p) freed\n", p_vertex);
		}
#endif
		
		struct he_vertex* p_next = p_vertex->p_next;

		if (p_closest_face) {
			if (p_vertices == p_hull->p_free_vertices) {
				qh_freelist_erase((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_vertex);
			}
			qh_freelist_set_head((struct qh_freelist_node**)&p_closest_face->p_qh_conflict_list, (struct qh_freelist_node*)p_vertex);
		} else if (p_vertices != p_hull->p_free_vertices) {
			qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_vertex);
		}
		
		p_vertex = p_next;
	}
}

struct he_vertex* qh_next_conflict_vertex(struct he_mesh* p_hull, struct he_face** pp_face) {
	struct he_vertex* p_conflict_vertex = 0;
	float dist_sq = -FLT_MAX;

	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		if (p_face->p_qh_conflict_list) {
			struct he_vertex* p_vertex = p_face->p_qh_conflict_list;
			while (p_vertex) {
				const float vertex_dist_sq = qh_face_vertex_dist_sq(p_face, p_vertex);

				if (vertex_dist_sq > dist_sq) {
					*pp_face = p_face;
					p_conflict_vertex = p_vertex;
					dist_sq = vertex_dist_sq;
				}

				p_vertex = p_vertex->p_next;
			}
		}

		p_face = p_face->p_next;
	}

	if (p_conflict_vertex) {
		qh_freelist_erase((struct qh_freelist_node**)&(*pp_face)->p_qh_conflict_list, (struct qh_freelist_node*)p_conflict_vertex);
	}

#if QH_DEBUG
	CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "NEXT CONFLICT VERTEX: (%p) face=(%p)\n", p_conflict_vertex, *pp_face);
#endif

	return p_conflict_vertex;
}

// todo: unmake recursive?
void qh_find_horizon(struct he_edge* p_edge, const struct he_vertex* p_eye, struct he_edge** pp_horizon_start) {
	p_edge->p_face->b_qh_processed = 1;

	const struct he_edge* p_starting_edge = p_edge;
	do {
		if (!p_edge->p_twin->p_face->b_qh_processed) {
			const int b_is_onside = qh_is_point_above_face_plane(p_edge->p_twin->p_face, p_eye->position, FLT_EPSILON);

			if (b_is_onside) {
				qh_find_horizon(p_edge->p_twin->p_next, p_eye, pp_horizon_start);
			} else {
				p_edge->p_qh_horizon_next = *pp_horizon_start;
				*pp_horizon_start = p_edge;
			}
		}
		p_edge = p_edge->p_next;
	} while (p_edge != p_starting_edge);
}

int qh_is_point_above_abc_plane(const float* p_a, const float* p_b, const float* p_c, const float* p_point, float threshold) {
	float normal[3];
	qh_compute_triangle_normal(p_a, p_b, p_c, normal);

	float rel_point[3];
	vec3_sub(p_point, p_a, rel_point);

	// CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "qh_is_point_above_abc_plane:\n\tabc=([%f, %f, %f], [%f, %f, %f], [%f, %f, %f])\n\tpoint=[%f, %f, %f], normal=[%f, %f, %f]\n\trel_point=[%f, %f, %f], threshold=%f, rel_point.normal=%f\n"
	// 	, p_a[0], p_a[1], p_a[2]
	// 	, p_b[0], p_b[1], p_b[2]
	// 	, p_c[0], p_c[1], p_c[2]
	// 	, p_point[0], p_point[1], p_point[2]
	// 	, normal[0], normal[1], normal[2]
	// 	, rel_point[0], rel_point[1], rel_point[2], -threshold, vec3_dot(rel_point, normal));
	
	return vec3_dot(rel_point, normal) > -threshold;
}

int qh_is_point_above_face_plane(const struct he_face* p_face, const float* p_point, float threshold) {
	return qh_is_point_above_abc_plane(
		p_face->p_edges->p_tail->position,
		p_face->p_edges->p_next->p_tail->position,
		p_face->p_edges->p_next->p_next->p_tail->position,
		p_point, threshold);
}

void qh_compute_face_center(const struct he_face* p_face, float* p_result) {
	vec3_set_s(0, p_result);

	size_t num_vertices = 0;

	struct he_edge* p_edge = p_face->p_edges;
	do {
		vec3_add(p_result, p_edge->p_tail->position, p_result);
		++num_vertices;
		p_edge = p_edge->p_next;
	} while(p_edge != p_face->p_edges);

	vec3_div_s(p_result, num_vertices, p_result);
}

int qh_edge_is_convex(const struct he_edge* p_edge, float threshold) {
	float face_center[3];

	qh_compute_face_center(p_edge->p_face, face_center);
	if (qh_is_point_above_face_plane(p_edge->p_twin->p_face, face_center, threshold)) {
		return 0;
	}

	qh_compute_face_center(p_edge->p_twin->p_face, face_center);
	if (qh_is_point_above_face_plane(p_edge->p_face, face_center, threshold)) {
		return 0;
	}

	return 1;
}

void qh_delete_hull_face(struct he_mesh* p_hull, struct he_face* p_face) {
	qh_freelist_erase((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)p_face);
	qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_faces, (struct qh_freelist_node*)p_face);
}

void qh_twin_edges(struct he_edge* p_edge_a, struct he_edge* p_edge_b) {
	p_edge_a->p_twin = p_edge_b;
	p_edge_b->p_twin = p_edge_a;
}

void qh_merge_edge_faces(struct he_mesh* p_hull, struct he_edge* p_edge) {
	struct he_face* p_face = p_edge->p_face;
	struct he_edge* p_edge2 = p_edge->p_twin;
	struct he_face* p_face2 = p_edge2->p_face;

#if QH_DEBUG
	CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Non-convex edge; merging (%p with %p, along edge %p / %p). Face pre-merge:\n", p_face, p_face2, p_edge, p_edge2);
	QH_DEBUG_LOG_FACE(p_face);
#endif

	struct he_edge* p_face2_edge = p_edge2->p_next;
	while (p_face2_edge != p_edge2) {
		p_face2_edge->p_face = p_face;
		p_face2_edge = p_face2_edge->p_next;
	}

	qh_join_edges(p_edge->p_next, p_edge2->p_prev);
	qh_join_edges(p_edge2->p_next, p_edge->p_prev);

	qh_freelist_append((struct qh_freelist_node**)&p_face->p_qh_conflict_list, (struct qh_freelist_node*)p_face2->p_qh_conflict_list);

	if (p_edge == p_face->p_edges) {
		p_face->p_edges = p_edge->p_prev->p_next;
	}

	qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge);
	qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge2);

	qh_delete_hull_face(p_hull, p_face2);
	
	qh_rebuild_face_plane(p_face);

#if QH_DEBUG
	CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\tFace post-merge:\n", p_face);
	QH_DEBUG_LOG_FACE(p_face);
#endif
}

void qh_fix_topology_errors(struct he_mesh* p_hull, struct he_edge* p_edge_i, struct he_edge* p_edge_o) {
	if (p_edge_i->p_twin->p_face != p_edge_o->p_twin->p_face) {
		return;
	}

	struct he_face* p_face = p_edge_o->p_face;

	if (p_edge_o->p_twin->p_prev == p_edge_i->p_twin->p_next) {
#if QH_DEBUG
		CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Topological error! Ingoing (%p) and outgoing (%p) merge edges pointing to same face (%p) (error 1)\n", p_edge_i, p_edge_o, p_edge_o->p_twin->p_face);
#endif
		struct he_edge* p_outer_edge = p_edge_o->p_twin->p_prev;

		struct he_edge* p_outer_edge_prev_old = p_outer_edge->p_prev;
		struct he_edge* p_outer_edge_next_old = p_outer_edge->p_next;

		qh_join_edges(p_outer_edge, p_edge_i->p_prev);
		qh_join_edges(p_edge_o->p_next, p_outer_edge);
		
		qh_freelist_append((struct qh_freelist_node**)&p_face->p_qh_conflict_list, (struct qh_freelist_node*)p_outer_edge->p_face->p_qh_conflict_list);
		
		if (p_outer_edge_prev_old->p_twin->p_face->p_edges == p_outer_edge_prev_old->p_twin) {
			p_outer_edge_prev_old->p_twin->p_face->p_edges  = p_outer_edge;
		}
		
		if (p_outer_edge_next_old->p_twin->p_face->p_edges == p_outer_edge_next_old->p_twin) {
			p_outer_edge_next_old->p_twin->p_face->p_edges = p_outer_edge;
		}

		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_outer_edge_prev_old->p_tail);
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_prev_old->p_twin);
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_prev_old);
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_next_old->p_twin);
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_next_old);

		qh_delete_hull_face(p_hull, p_outer_edge->p_face);

		p_outer_edge->p_face = p_face;

		qh_rebuild_face_plane(p_face);
				
		qh_fix_topology_errors(p_hull, p_outer_edge, p_outer_edge->p_next);
		qh_fix_topology_errors(p_hull, p_outer_edge->p_prev, p_outer_edge);
	} else {
#if QH_DEBUG
		CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Topological error! Ingoing (%p) and outgoing (%p) merge edges pointing to same face (%p) (error 2)\n", p_edge_i, p_edge_o, p_edge_o->p_twin->p_face);
#endif
		struct he_edge* p_edge_r1 = p_edge_o;
		struct he_edge* p_edge_s1 = p_edge_i;
		struct he_edge* p_edge_r2 = p_edge_s1->p_twin;
		struct he_edge* p_edge_s2 = p_edge_r1->p_twin;

		qh_join_edges(p_edge_r1->p_next, p_edge_s1);
		qh_join_edges(p_edge_r2->p_next, p_edge_s2);

		qh_twin_edges(p_edge_s1, p_edge_s2);

		if (p_edge_r1->p_face->p_edges == p_edge_r1) {
			p_edge_r1->p_face->p_edges = p_edge_r1->p_next;
		}

		if (p_edge_r2->p_face->p_edges == p_edge_r2) {
			p_edge_r2->p_face->p_edges = p_edge_r2->p_next;
		}

		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_edge_r1->p_tail);
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge_r1);
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge_r2);
	}

#if QH_DEBUG
	QH_DEBUG_LOG_HULL(p_hull, "Post-topology-fix");
#endif
}

void qh_rebuild_face_plane(struct he_face* p_face) {
	float plane[4] = {0};
	float sum[3] = {0};
	size_t num_vertices = 0;
	
	struct he_edge* p_edge = p_face->p_edges;
	do {
		const float* p_p1 = p_edge->p_tail->position;
		const float* p_p2 = p_edge->p_next->p_tail->position;

		plane[0] += (p_p1[1] - p_p2[1]) * (p_p1[2] + p_p2[2]);
		plane[1] += (p_p1[2] - p_p2[2]) * (p_p1[0] + p_p2[0]);
		plane[2] += (p_p1[0] - p_p2[0]) * (p_p1[1] + p_p2[1]);
		
		vec3_add(sum, p_p1, sum);
		
		++num_vertices;
		
		p_edge = p_edge->p_next;
	} while(p_edge != p_face->p_edges);
	
	const float len = vec3_len(plane);
	
	plane[3] = -vec3_dot(sum, plane) / (len * num_vertices);

	vec3_div_s(plane, len, plane);

	float original_normal[3];
	qh_compute_triangle_normal(p_face->p_edges->p_tail->position, p_face->p_edges->p_next->p_tail->position, p_face->p_edges->p_next->p_next->p_tail->position, original_normal);

	// CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Rebuilding face plane: original_normal=[%f, %f, %f], new_plane=([%f, %f, %f], %f)\n", original_normal[0], original_normal[1], original_normal[2], plane[0], plane[1], plane[2], plane[3]);
	
	p_edge = p_face->p_edges;
	do {		
		float offset[3];
		vec3_mul_s(plane, vec3_dot(plane, p_edge->p_tail->position) + plane[3], offset);

		// CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\tOld vertex position=[%f, %f, %f]\n", p_edge->p_tail->position[0], p_edge->p_tail->position[1], p_edge->p_tail->position[2]);

		vec3_sub(p_edge->p_tail->position, offset, p_edge->p_tail->position);

		// CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\tNew vertex position=[%f, %f, %f]\n", p_edge->p_tail->position[0], p_edge->p_tail->position[1], p_edge->p_tail->position[2]);

		p_edge = p_edge->p_next;
	} while(p_edge != p_face->p_edges);
}

void qh_construct_horizon_faces(struct he_mesh* p_hull, struct he_edge* p_horizon_start, struct he_vertex* p_eye, float threshold) {
#if QH_DEBUG
	CX_DBG_LOG(CX_LOG_CAT_QH, "HORIZON EDGES:\n");
	struct he_edge* p_temp_horizon_edge = p_horizon_start;
	while(p_temp_horizon_edge) {
		CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\tEdge (%p)\n", p_temp_horizon_edge);
		p_temp_horizon_edge = p_temp_horizon_edge->p_qh_horizon_next;
	}
#endif

	struct he_face* p_new_faces = 0;

	struct he_edge* p_horizon_edge = p_horizon_start;
	
	qh_freelist_set_head((struct qh_freelist_node**)&p_new_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, p_horizon_edge->p_tail, p_horizon_edge->p_next->p_tail, p_eye));

	qh_twin_edges(p_horizon_edge->p_twin, p_new_faces->p_edges);

	p_horizon_edge = p_horizon_edge->p_qh_horizon_next;

	struct he_face* p_new_faces_back = p_new_faces;

	while (p_horizon_edge) {
		qh_freelist_set_head((struct qh_freelist_node**)&p_new_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, p_horizon_edge->p_tail, p_horizon_edge->p_next->p_tail, p_eye));

		qh_twin_edges(p_horizon_edge->p_twin, p_new_faces->p_edges);
		qh_twin_edges(p_new_faces->p_edges->p_next, p_new_faces->p_next->p_edges->p_next->p_next);
		
		p_horizon_edge = p_horizon_edge->p_qh_horizon_next;
	}

	qh_twin_edges(p_new_faces_back->p_edges->p_next, p_new_faces->p_edges->p_next->p_next);

	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		struct he_face* p_next = p_face->p_next;

		if (p_face->b_qh_processed) {
#if QH_DEBUG
			CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Deleting redundant face (%p) and redistributing conflict vertices:\n", p_face);
			struct he_vertex* p_vertex_debug = p_face->p_qh_conflict_list;
			while (p_vertex_debug) {
				CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "\t\tConflict Vertex (%p) [%f, %f, %f], p_prev=%p, p_next=%p\n",
				p_vertex_debug, p_vertex_debug->position[0], p_vertex_debug->position[1], p_vertex_debug->position[2], p_vertex_debug->p_prev, p_vertex_debug->p_next);
				p_vertex_debug = p_vertex_debug->p_next;
			}
#endif

			qh_distribute_conflict_vertices(p_hull, p_new_faces, p_face->p_qh_conflict_list, threshold);

			struct he_edge* p_edge = p_face->p_edges;
			do {
				struct he_edge* p_next_edge = p_edge->p_next;
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge);
				p_edge = p_next_edge;
			} while(p_edge != p_face->p_edges);

			qh_delete_hull_face(p_hull, p_face);
		}

		p_face = p_next;
	}
	
	p_hull->p_faces->p_prev = p_new_faces_back;
	p_new_faces_back->p_next = p_hull->p_faces;
	
	p_hull->p_faces = p_new_faces;
	
#if QH_DEBUG
	QH_DEBUG_LOG_HULL(p_hull, "Post-horizon-construction");
#endif
}

void qh_merge_coplanars(struct he_mesh* p_hull, float threshold) {
	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		struct he_face* p_next = p_face->p_next;
		struct he_edge* p_edge = p_face->p_edges;
		do {
			
			if (qh_edge_is_convex(p_edge, threshold)) {
				// CX_DBG_LOG(CX_LOG_CAT_QH, "EDGE IS CONVEX\n");
				// QH_DEBUG_LOG_FACE(p_edge->p_face);
				// QH_DEBUG_LOG_FACE(p_edge->p_twin->p_face);
				p_edge = p_edge->p_next;
				continue;
			}
			
			// CX_DBG_LOG(CX_LOG_CAT_QH, "EDGE IS NON-CONVEX\n");
			// QH_DEBUG_LOG_FACE(p_edge->p_face);
			// QH_DEBUG_LOG_FACE(p_edge->p_twin->p_face);
			
			struct he_edge* p_edge1_i = p_edge->p_twin->p_prev;
			struct he_edge* p_edge1_o = p_edge->p_next;
			struct he_edge* p_edge2_i = p_edge->p_prev;
			struct he_edge* p_edge2_o = p_edge->p_twin->p_next;
			
			qh_merge_edge_faces(p_hull, p_edge);

			qh_fix_topology_errors(p_hull, p_edge1_i, p_edge1_o);
			qh_fix_topology_errors(p_hull, p_edge2_i, p_edge2_o);
			
			p_next = p_hull->p_faces;
			break;
		} while(p_edge != p_face->p_edges);
		p_face = p_next;
	}
	
#if QH_DEBUG
	//QH_DEBUG_LOG_HULL(p_hull, "Post-merge");
#endif
}

static int qh_sort_cmp(const float* p_a, const float* p_b) {
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

void qh_purge_duplicate_input_points(float* p_point_cloud, size_t* p_num_points) {
	qsort(p_point_cloud, *p_num_points, sizeof(float) * 3, (void*)qh_sort_cmp);

	size_t m = 1;
    for (size_t i = 1; i < *p_num_points; ++i) {
		float* p_a = &p_point_cloud[i * 3];
		float* p_b = &p_point_cloud[(m - 1) * 3];
        if (!vec3_cmp(p_a, p_b)) {
            if (i != m) {
				vec3_set(p_a, &p_point_cloud[m * 3]);
            }
            ++m;
        }
    }

	CX_DBG_LOG_FMT(CX_LOG_CAT_QH, "Purged %d point duplicates\n", *p_num_points - m);

    *p_num_points = m;
}

void quickhull(float* p_point_cloud, size_t num_points, struct he_mesh* p_hull) {
	qh_purge_duplicate_input_points(p_point_cloud, &num_points);
	
	const size_t max_edges = (3 * num_points - 6) * 4;
	const size_t max_faces = (2 * num_points - 4) * 2;
	const size_t size_chunk0 = sizeof(struct he_vertex) * num_points;
	const size_t size_chunk1 = sizeof(struct he_edge) * max_edges;
	const size_t size_chunk2 = sizeof(struct he_face) * max_faces;
	
	cx_log_fmt(CX_LOG_TRACE, CX_LOG_CAT_QH, "Generating convex hull for point cloud: num_points=%d, max_edges=%d, max_faces=%d\n", num_points, max_edges, max_faces);

	*p_hull = (struct he_mesh){0};

	p_hull->p_buffer = calloc(1, size_chunk0 + size_chunk1 + size_chunk2);

	p_hull->p_free_vertices = p_hull->p_buffer;
	p_hull->p_free_edges = (void*)((char*)p_hull->p_free_vertices + size_chunk0);
	p_hull->p_free_faces = (void*)((char*)p_hull->p_free_edges + size_chunk1);

	qh_freelist_init((struct qh_freelist_node*)p_hull->p_free_vertices, sizeof(*p_hull->p_free_vertices), num_points);
	qh_freelist_init((struct qh_freelist_node*)p_hull->p_free_edges, sizeof(*p_hull->p_free_edges), max_edges);
	qh_freelist_init((struct qh_freelist_node*)p_hull->p_free_faces, sizeof(*p_hull->p_free_faces), max_faces);

	struct he_vertex* initial_hull_vertices[4];
	float threshold;

	qh_find_initial_hull_vertices(p_point_cloud, num_points, p_hull, initial_hull_vertices, &threshold);

	const int b_is_acb = qh_is_point_above_abc_plane(
		initial_hull_vertices[0]->position,
		initial_hull_vertices[1]->position,
		initial_hull_vertices[2]->position,
		initial_hull_vertices[3]->position, 0);

	if (b_is_acb) {
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[0], initial_hull_vertices[2], initial_hull_vertices[1]));
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[0], initial_hull_vertices[3], initial_hull_vertices[2]));
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[0], initial_hull_vertices[1], initial_hull_vertices[3]));
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[1], initial_hull_vertices[2], initial_hull_vertices[3]));
	} else {
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[0], initial_hull_vertices[1], initial_hull_vertices[2]));
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[0], initial_hull_vertices[2], initial_hull_vertices[3]));
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[0], initial_hull_vertices[3], initial_hull_vertices[1]));
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, initial_hull_vertices[1], initial_hull_vertices[3], initial_hull_vertices[2]));
	}

	qh_find_initial_hull_half_edge_pairs(p_hull);

	qh_distribute_conflict_vertices(p_hull, p_hull->p_faces, p_hull->p_free_vertices, threshold);

#if QH_DEBUG
	QH_DEBUG_LOG_HULL(p_hull, "Initial hull");
#endif

	size_t i = 0;
	
	struct he_face* p_conflict_face = 0;
	struct he_vertex* p_conflict_vertex = qh_next_conflict_vertex(p_hull, &p_conflict_face);
	while (p_conflict_vertex) {
#if QH_DEBUG
		// if (i == 2) {
		// 	break;
		// }
#endif

		struct he_edge* p_horizon_start = 0;
		qh_find_horizon(p_conflict_face->p_edges, p_conflict_vertex, &p_horizon_start);
		
		qh_construct_horizon_faces(p_hull, p_horizon_start, p_conflict_vertex, threshold);

		qh_merge_coplanars(p_hull, threshold);

		++i;
		
		p_conflict_vertex = qh_next_conflict_vertex(p_hull, &p_conflict_face);
		
		// QH_DEBUG_LOG_HULL(p_hull, "Post-next-conflict-vertex");
	}
	
	cx_log_fmt(CX_LOG_TRACE, CX_LOG_CAT_QH, "Completed in %d steps. Epsilon=%f\n", i, threshold);
}

void quickhull_static_mesh(const struct static_mesh* p_static_mesh, struct he_mesh* p_result) {
	size_t num_vertices = 0;

	for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		const struct mesh_primitive* p_primitive = &p_static_mesh->p_primitives[i];
		num_vertices += p_primitive->vertex_count;
	}

	float* point_cloud_points = malloc(num_vertices * sizeof(float) * 3);

	for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		const struct mesh_primitive* p_primitive = &p_static_mesh->p_primitives[i];

		const struct vertex_attribute* p_position_attribute = 0;

		for (size_t a = 0; a < p_primitive->num_attributes; ++a) {
			const struct vertex_attribute* p_attribute = &p_primitive->p_attributes[a];
			if (p_attribute->index == 0) {
				p_position_attribute = p_attribute;
				break;
			}
		}

		if (!p_position_attribute) {
			continue;
		}

		const struct vertex_buffer* p_position_buffer = &p_primitive->p_vertex_buffers[p_position_attribute->vertex_buffer_index];

		for (size_t v = 0; v < p_primitive->vertex_count; ++v) {
			const float* p_v = (float*)((char*)p_position_buffer->p_bytes + p_position_attribute->layout.offset + (p_position_attribute->layout.stride * v));
			vec3_set(p_v, &point_cloud_points[v * 3]);
		}
	}

	quickhull(point_cloud_points, num_vertices, p_result);

	free(point_cloud_points);
}

void quickhull_free(struct he_mesh* p_mesh) {
	free(p_mesh->p_buffer);
	*p_mesh = (struct he_mesh){0};
}