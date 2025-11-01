#include <float.h>
#include <malloc.h>

#include "logging.h"
#include "math_utils.h"
#include "mesh.h"
#include "quickhull.h"
#include "static_mesh.h"
#include "vector.h"

struct qh_freelist_node {
	struct qh_freelist_node* p_prev;
	struct qh_freelist_node* p_next;
};

static void qh_freelist_init(struct qh_freelist_node* p_head, size_t node_size, size_t n) {
	struct qh_freelist_node* p_node = p_head;

	p_node->p_prev = 0;

	for (size_t i = 0; i < n - 1; ++i) {
		p_node->p_next = (struct qh_freelist_node*)((char*)p_node + node_size);
		p_node->p_next->p_prev = p_node;
		p_node = p_node->p_next;
	}

	p_node->p_next = 0;
}

static struct qh_freelist_node* qh_freelist_get(struct qh_freelist_node** pp_list) {
	if (!*pp_list) {
		log_msg(LOG_DEBUG, "quickhull", "Freelist empty!\n");
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

static void qh_freelist_set_head(struct qh_freelist_node** pp_list, struct qh_freelist_node* p_node) {
	p_node->p_next = *pp_list;
	p_node->p_prev = 0;

	if (p_node->p_next) {
		p_node->p_next->p_prev = p_node;
	}
	
	*pp_list = p_node;
}

static void qh_compute_face_normal(const struct he_face* p_face, float* p_result) {
	float tmp[3];

	vec3_sub(p_face->p_edges->p_next->p_tail->position, p_face->p_edges->p_tail->position, tmp);
	vec3_sub(p_face->p_edges->p_next->p_next->p_tail->position, p_face->p_edges->p_tail->position, p_result);

	// log_msg(LOG_DEBUG, "quickhull", "Computing face (%p) normal: a=[%f, %f, %f], b=[%f, %f, %f], c=[%f, %f, %f]\n", p_face
	// 	, p_face->p_edges->p_tail->position[0], p_face->p_edges->p_tail->position[1], p_face->p_edges->p_tail->position[2]
	// 	, p_face->p_edges->p_next->p_tail->position[0], p_face->p_edges->p_next->p_tail->position[1], p_face->p_edges->p_next->p_tail->position[2]
	// 	, p_face->p_edges->p_next->p_next->p_tail->position[0], p_face->p_edges->p_next->p_next->p_tail->position[1], p_face->p_edges->p_next->p_next->p_tail->position[2]);

	// log_msg(LOG_DEBUG, "quickhull", "\ta->b=[%f, %f, %f], a->c=[%f, %f, %f]\n", p_face, tmp[0], tmp[1], tmp[2], p_result[0], p_result[1], p_result[2]);

	vec3_cross(tmp, p_result, p_result);
	vec3_norm(p_result, p_result);

	// log_msg(LOG_DEBUG, "quickhull", "\tNormal=[%f, %f, %f]\n", p_result[0], p_result[1], p_result[2]);
}

static float qh_face_vertex_dist_sq(const struct he_face* p_face, const struct he_vertex* p_vertex) {
	// return triangle_point_dist_sq(
	// 	p_face->p_edges->p_tail->position,
	// 	p_face->p_edges->p_next->p_tail->position,
	// 	p_face->p_edges->p_next->p_next->p_tail->position,
	// 	p_vertex->position);

	// todo: we might be able to get away with not normalizing the normal here since we only want dist_sq

	float normal[3];
	qh_compute_face_normal(p_face, normal);

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

// Finds the four vertices of the initial tetrahedron hull from a given point cloud.
// As a side effect, also copies the point cloud position data in to the hull's vertex list.
static void qh_find_initial_hull_vertices(const float* p_point_cloud, size_t num_points, struct he_mesh* p_hull, struct he_vertex** pp_vertices) {
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

	log_msg(LOG_DEBUG, "quickhull", "Point cloud extremes:\n\tx_axis=([%f, %f, %f], [%f, %f, %f])\n\ty_axis=([%f, %f, %f], [%f, %f, %f])\n\tz_axis=([%f, %f, %f], [%f, %f, %f])\n"
		, p_point_cloud[extremes[0] * 3 + 0], p_point_cloud[extremes[0] * 3 + 1], p_point_cloud[extremes[0] * 3 + 2], p_point_cloud[extremes[1] * 3 + 0], p_point_cloud[extremes[1] * 3 + 1], p_point_cloud[extremes[1] * 3 + 2]
		, p_point_cloud[extremes[2] * 3 + 0], p_point_cloud[extremes[2] * 3 + 1], p_point_cloud[extremes[2] * 3 + 2], p_point_cloud[extremes[3] * 3 + 0], p_point_cloud[extremes[3] * 3 + 1], p_point_cloud[extremes[3] * 3 + 2]
		, p_point_cloud[extremes[4] * 3 + 0], p_point_cloud[extremes[4] * 3 + 1], p_point_cloud[extremes[4] * 3 + 2], p_point_cloud[extremes[5] * 3 + 0], p_point_cloud[extremes[5] * 3 + 1], p_point_cloud[extremes[5] * 3 + 2]);

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
		// log_msg(LOG_DEBUG, "quickhull", "Segment: [%f, %f, %f]->[%f, %f, %f], Point: [%f, %f, %f], distance: %f\n"
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
		// log_msg(LOG_DEBUG, "quickhull", "Triangle: [%f, %f, %f]->[%f, %f, %f]->[%f, %f, %f], Point: [%f, %f, %f], distance: %f\n"
		// 	, p_point_cloud[iv[0] * 3], p_point_cloud[iv[0] * 3 + 1], p_point_cloud[iv[0] * 3 + 2]
		// 	, p_point_cloud[iv[1] * 3], p_point_cloud[iv[1] * 3 + 1], p_point_cloud[iv[1] * 3 + 2]
		// 	, p_point_cloud[iv[2] * 3], p_point_cloud[iv[2] * 3 + 1], p_point_cloud[iv[2] * 3 + 2]
		// 	, p[0], p[1], p[2]
		// 	, d);
	}

	// log_msg(LOG_DEBUG, "quickhull", "Initial vertices: [%f, %f, %f], [%f, %f, %f], [%f, %f, %f], [%f, %f, %f]\n"
	// 	, p_point_cloud[iv[0] * 3], p_point_cloud[iv[0] * 3 + 1], p_point_cloud[iv[0] * 3 + 2]
	// 	, p_point_cloud[iv[1] * 3], p_point_cloud[iv[1] * 3 + 1], p_point_cloud[iv[1] * 3 + 2]
	// 	, p_point_cloud[iv[2] * 3], p_point_cloud[iv[2] * 3 + 1], p_point_cloud[iv[2] * 3 + 2]
	// 	, p_point_cloud[iv[3] * 3], p_point_cloud[iv[3] * 3 + 1], p_point_cloud[iv[3] * 3 + 2]);

	pp_vertices[0] = &p_hull->p_free_vertices[iv[0]];
	pp_vertices[1] = &p_hull->p_free_vertices[iv[1]];
	pp_vertices[2] = &p_hull->p_free_vertices[iv[2]];
	pp_vertices[3] = &p_hull->p_free_vertices[iv[3]];

	for (size_t i = 0; i < 4; ++i) {
		log_msg(LOG_DEBUG, "quickhull", "Initial vertex: (%d:%d) [%f, %f, %f]\n", i, iv[i]
			, pp_vertices[i]->position[0], pp_vertices[i]->position[1], pp_vertices[i]->position[2]);

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

	log_msg(LOG_DEBUG, "quickhull", "Initial vertices: [%f, %f, %f], [%f, %f, %f], [%f, %f, %f], [%f, %f, %f]\n"
		, pp_vertices[0]->position[0], pp_vertices[0]->position[1], pp_vertices[0]->position[2]
		, pp_vertices[1]->position[0], pp_vertices[1]->position[1], pp_vertices[1]->position[2]
		, pp_vertices[2]->position[0], pp_vertices[2]->position[1], pp_vertices[2]->position[2]
		, pp_vertices[3]->position[0], pp_vertices[3]->position[1], pp_vertices[3]->position[2]);
}

static void qh_join_edges(struct he_edge* p_edge_head, struct he_edge* p_edge_tail) {
	p_edge_head->p_prev = p_edge_tail;
	p_edge_tail->p_next = p_edge_head;
}

static struct he_face* qh_construct_new_face(struct he_mesh* p_hull, struct he_vertex* p_fa, struct he_vertex* p_fb, struct he_vertex* p_fc) {
	log_msg(LOG_DEBUG, "quickhull", "New face: [%f, %f, %f]->[%f, %f, %f]->[%f, %f, %f]\n"
		, p_fa->position[0], p_fa->position[1], p_fa->position[2]
		, p_fb->position[0], p_fb->position[1], p_fb->position[2]
		, p_fc->position[0], p_fc->position[1], p_fc->position[2]);

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

static int qh_is_facing_vertex(const struct he_vertex* p_fa, const struct he_vertex* p_fb, const struct he_vertex* p_fc, const struct he_vertex* p_p) {
	float tmp1[3];
	float tmp2[3];

	vec3_sub(p_fb->position, p_fa->position, tmp1);
	vec3_sub(p_fc->position, p_fa->position, tmp2);

	vec3_cross(tmp1, tmp2, tmp1);
	vec3_norm(tmp1, tmp1);

	vec3_sub(p_p->position, p_fa->position, tmp2);

	// log_msg(LOG_DEBUG, "quickhull", "qh_is_facing_vertex:\n\ta=[%f, %f, %f]\n\tb=[%f, %f, %f]\n\tc=[%f, %f, %f]\n\tp=[%f, %f, %f]\n\tnorm=[%f, %f, %f], norm.p=%f\n"
	// 	, p_fa->position[0], p_fa->position[1], p_fa->position[2]
	// 	, p_fb->position[0], p_fb->position[1], p_fb->position[2]
	// 	, p_fc->position[0], p_fc->position[1], p_fc->position[2]
	// 	, p_p->position[0], p_p->position[1], p_p->position[2]
	// 	, norm[0], norm[1], norm[2], vec3_dot(norm, p_p->position));

	return vec3_dot(tmp1, tmp2) > 0;
}

static void qh_find_initial_hull_half_edge_pairs(struct he_mesh* p_hull) {
	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		struct he_edge* p_edge = p_face->p_edges;
		do {
			struct he_face* p_face_other = p_hull->p_faces;
			while (!p_edge->p_twin && p_face_other) {
				if (p_face_other != p_face) { // todo: can we just break here?
					struct he_edge* p_edge_other = p_face_other->p_edges;
					do {
						if (!p_edge_other->p_twin) {
							if (p_edge->p_tail == p_edge_other->p_next->p_tail && p_edge->p_next->p_tail == p_edge_other->p_tail) {
								p_edge->p_twin = p_edge_other;
								p_edge_other->p_twin = p_edge;
								break;
							}
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

static void qh_distribute_conflict_vertices(struct he_mesh* p_hull, struct he_face* p_faces, struct he_vertex* p_vertices) {
	// log_msg(LOG_DEBUG, "quickhull", "Distributing vertices (%p) to face conflict vertices lists...\n", p_vertices);

	struct he_vertex* p_vertex = p_vertices;
	while (p_vertex) {
		struct he_face* p_closest_face = 0;
		float closest_face_dist_sq = FLT_MAX;

		struct he_face* p_face = p_faces;
		while (p_face) {
			// todo: actually here we should check if vertex is co-planar, also

			const int b_is_onside = qh_is_facing_vertex(
				p_face->p_edges->p_tail,
				p_face->p_edges->p_next->p_tail,
				p_face->p_edges->p_next->p_next->p_tail,
				p_vertex);

			// log_msg(LOG_DEBUG, "quickhull", "is_facing_trangle(\n\ta=[%f, %f, %f],\n\tb=[%f, %f, %f],\n\tc=[%f, %f, %f],\n\tp=[%f, %f, %f])=%d\n",
			// 	p_face->p_edges->p_tail->position[0], p_face->p_edges->p_tail->position[1], p_face->p_edges->p_tail->position[2],
			// 	p_face->p_edges->p_next->p_tail->position[0], p_face->p_edges->p_next->p_tail->position[1], p_face->p_edges->p_next->p_tail->position[2],
			// 	p_face->p_edges->p_next->p_next->p_tail->position[0], p_face->p_edges->p_next->p_next->p_tail->position[1], p_face->p_edges->p_next->p_next->p_tail->position[2],
			// 	p_vertex->position[0], p_vertex->position[1], p_vertex->position[2], b_is_onside);

			if (b_is_onside) {
				const float face_dist_sq = qh_face_vertex_dist_sq(p_face, p_vertex);

				if (!FLT_CMP(face_dist_sq, 0) && face_dist_sq < closest_face_dist_sq) {
					closest_face_dist_sq = face_dist_sq;
					p_closest_face = p_face;
				}
			}

			p_face = p_face->p_next;
		}

		// if (p_closest_face) {
		// 	log_msg(LOG_DEBUG, "quickhull", "Adding vertex [%f, %f, %f] to face (%p) conflict list\n", p_vertex->position[0], p_vertex->position[1], p_vertex->position[2], p_closest_face);
		// } else {
		// 	log_msg(LOG_DEBUG, "quickhull", "Vertex [%f, %f, %f] not being added to any conflict list\n", p_vertex->position[0], p_vertex->position[1], p_vertex->position[2]);
		// }

		struct he_vertex** pp_freelist = p_closest_face ? &p_closest_face->p_qh_conflict_list : &p_hull->p_free_vertices;
		
		struct he_vertex* p_next = p_vertex->p_next;
		
		if (*pp_freelist != p_vertex) {
			qh_freelist_set_head((struct qh_freelist_node**)pp_freelist, (struct qh_freelist_node*)p_vertex);
		} 
		
		p_vertex = p_next;
	}

	// log_msg(LOG_DEBUG, "quickhull", "done.\n");
}

static struct he_vertex* qh_next_conflict_vertex(struct he_face* p_face) {
	// log_msg(LOG_DEBUG, "quickhull", "Finding next conflict vertex for face (%p)...\n", p_face);

	struct he_vertex* p_eye = 0;
	float eye_dist_sq = -FLT_MAX;

	struct he_vertex* p_vertex = p_face->p_qh_conflict_list;
	while (p_vertex) {
		const float vertex_dist_sq = qh_face_vertex_dist_sq(p_face, p_vertex);

		// log_msg(LOG_DEBUG, "quickhull", "\tConflict vertex [%f, %f, %f] (%p) dist_sq: %f\n", p_vertex->position[0], p_vertex->position[1], p_vertex->position[2], p_vertex, vertex_dist_sq);

		if (vertex_dist_sq > eye_dist_sq) {
			eye_dist_sq = vertex_dist_sq;
			p_eye = p_vertex;
		}

		p_vertex = p_vertex->p_next;
	}

	return p_eye;
}

static void qh_find_horizon(struct he_edge* p_edge, const struct he_vertex* p_eye, struct he_edge** pp_horizon_start) {
	// todo: unmake recursive?
	log_msg(LOG_DEBUG, "quickhull", "Processing face (%p) edges...\n", p_edge->p_face);

	p_edge->p_face->b_qh_processed = 1;

	const struct he_edge* p_starting_edge = p_edge;
	do {
		if (!p_edge->p_twin->p_face->b_qh_processed) {
			const int b_is_onside = qh_is_facing_vertex(
				p_edge->p_twin->p_face->p_edges->p_tail,
				p_edge->p_twin->p_face->p_edges->p_next->p_tail,
				p_edge->p_twin->p_face->p_edges->p_next->p_next->p_tail,
				p_eye);

			if (b_is_onside) {
				// log_msg(LOG_DEBUG, "quickhull", "\tEdge (%p): Twin face (%p) is visible. Stepping in to...\n", p_edge, p_edge->p_twin->p_face);
				qh_find_horizon(p_edge->p_twin->p_next, p_eye, pp_horizon_start);
			} else {
				// log_msg(LOG_DEBUG, "quickhull", "\tEdge (%p): Twin face (%p) is not visible. Found horizon edge. (a=[%f, %f, %F], b=[%f, %f, %f])\n", p_edge, p_edge->p_twin->p_face, p_edge->p_tail->position[0], p_edge->p_tail->position[1], p_edge->p_tail->position[2], p_edge->p_next->p_tail->position[0], p_edge->p_next->p_tail->position[1], p_edge->p_next->p_tail->position[2]);
				p_edge->p_qh_horizon_next = *pp_horizon_start;
				*pp_horizon_start = p_edge;
			}
		} // else {
			// log_msg(LOG_DEBUG, "quickhull", "\tEdge (%p): Twin face (%p) already processed. Skipping.\n", p_edge, p_edge->p_twin->p_face);
		// }
		p_edge = p_edge->p_next;
	} while (p_edge != p_starting_edge);
}

static int qh_is_point_above_face_plane(const struct he_face* p_face, const float* p_point, float threshold) {
	// todo: think we can get away without sqrt normal here
	float normal[3];
	qh_compute_face_normal(p_face, normal);

	float rel_point[3];
	vec3_sub(p_point, p_face->p_edges->p_tail->position, rel_point);

	// log_msg(LOG_DEBUG, "quickhull", "Face (%p)\n\tnorm=[%f, %f, %f]\n\tcenter=[%f, %f, %f]\n\tcenter_local=[%f, %f, %f]\n", p_face, normal[0], normal[1], normal[2], p_point[0], p_point[1], p_point[2], rel_point[0], rel_point[1], rel_point[2]);
	
	return vec3_dot(rel_point, normal) > -threshold;
}

static void qh_compute_face_center(const struct he_face* p_face, float* p_result) {
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

static int qh_edge_is_convex(const struct he_edge* p_edge, float threshold) {
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

static void qh_delete_hull_face(struct he_mesh* p_hull, struct he_face* p_face) {
	if (p_face->p_prev) {
		p_face->p_prev->p_next = p_face->p_next;
	} else {
		p_hull->p_faces = p_face->p_next;
	}

	if (p_face->p_next) {
		p_face->p_next->p_prev = p_face->p_prev;
	}

	qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_faces, (struct qh_freelist_node*)p_face);
}

static void qh_twin_edges(struct he_edge* p_edge_a, struct he_edge* p_edge_b) {
	p_edge_a->p_twin = p_edge_b;
	p_edge_b->p_twin = p_edge_a;
}

static void qh_merge_edge_faces(struct he_edge* p_edge) {
	struct he_face* p_face = p_edge->p_face;
	struct he_edge* p_edge2 = p_edge->p_twin;
	struct he_face* p_face2 = p_edge2->p_face;

	log_msg(LOG_DEBUG, "quickhull", "Non-convex edge; merging (%p with %p, along edge %p / %p)...\n", p_face, p_face2, p_edge, p_edge2);

	struct he_edge* p_face2_edge = p_edge2->p_next;
	while (p_face2_edge != p_edge2) {
		p_face2_edge->p_face = p_face;
		p_face2_edge = p_face2_edge->p_next;
	}

	qh_join_edges(p_edge->p_next, p_edge2->p_prev);
	qh_join_edges(p_edge2->p_next, p_edge->p_prev);
	
	struct he_vertex** pp_conflict_list_back = &p_face->p_qh_conflict_list;
	while (*pp_conflict_list_back) {
		pp_conflict_list_back = &(*pp_conflict_list_back)->p_next;
	}
	*pp_conflict_list_back = p_face2->p_qh_conflict_list;

	if (p_edge == p_face->p_edges) {
		p_face->p_edges = p_edge->p_prev->p_next;
	}

	log_msg(LOG_DEBUG, "quickhull", "\tFACE POST-MERGE (%p)\n", p_face);
	log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d, p_prev=%p, p_next=%p\n", p_face->b_qh_processed, p_face->p_prev, p_face->p_next);
	struct he_edge* p_temp_edge = p_face->p_edges;
	do {
		log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p, p_twin=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next, p_temp_edge->p_twin);
		p_temp_edge = p_temp_edge->p_next;
	} while(p_temp_edge != p_face->p_edges);

	struct he_vertex* p_temp_vertex = p_face->p_qh_conflict_list;
	while (p_temp_vertex) {
		log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
		p_temp_vertex = p_temp_vertex->p_next;
	}
}

static void qh_merge_face_edge_faces(struct he_mesh* p_hull, struct he_face* p_face, float threshold) {
	size_t DEBUG_iters = 0;
	size_t DEBUG_MAX_ITERS = 1;

	{
		log_msg(LOG_DEBUG, "quickhull", "\tFACE PRE-MERGE ATTEMPTS (%p)\n", p_face);
		log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d, p_prev=%p, p_next=%p\n", p_face->b_qh_processed, p_face->p_prev, p_face->p_next);
		struct he_edge* p_temp_edge = p_face->p_edges;
		do {
			log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p, p_twin=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next, p_temp_edge->p_twin);
			p_temp_edge = p_temp_edge->p_next;
		} while(p_temp_edge != p_face->p_edges);

		struct he_vertex* p_temp_vertex = p_face->p_qh_conflict_list;
		while (p_temp_vertex) {
			log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
			p_temp_vertex = p_temp_vertex->p_next;
		}
	}

	struct he_edge* p_edge = p_face->p_edges;
	do {
		if (qh_edge_is_convex(p_edge, threshold)) {
			p_edge = p_edge->p_next;
			// log_msg(LOG_DEBUG, "quickhull", "Convex edge; doing nothing.\n");
			continue;
		}
		
		// if (DEBUG_iters == DEBUG_MAX_ITERS) {
		// 	return;
		// }
		// ++DEBUG_iters;
		
		log_msg(LOG_DEBUG, "quickhull", "Non-convex edge; merging (%p with %p)...\n", p_face, p_edge->p_twin->p_face);
		
		struct he_edge* p_edge2 = p_edge->p_twin;
		struct he_face* p_face2 = p_edge2->p_face;

		struct he_edge* p_face2_edge = p_edge2->p_next;
		while (p_face2_edge != p_edge2) {
			p_face2_edge->p_face = p_face;
			p_face2_edge = p_face2_edge->p_next;
		}

		qh_join_edges(p_edge->p_next, p_edge2->p_prev);
		qh_join_edges(p_edge2->p_next, p_edge->p_prev);
		
		{
			struct he_vertex** pp_conflict_list_back = &p_face->p_qh_conflict_list;
			while (*pp_conflict_list_back) {
				pp_conflict_list_back = &(*pp_conflict_list_back)->p_next;
			}
			*pp_conflict_list_back = p_face2->p_qh_conflict_list;
		}

		int b_restart = 0;

		if (p_edge == p_face->p_edges) {
			p_face->p_edges = p_edge->p_prev->p_next;
			b_restart = 1;
		}

		{
			log_msg(LOG_DEBUG, "quickhull", "\tFACE POST-MERGE (%p)\n", p_face);
				log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d, p_prev=%p, p_next=%p\n", p_face->b_qh_processed, p_face->p_prev, p_face->p_next);
			struct he_edge* p_temp_edge = p_face->p_edges;
			do {
				log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p, p_twin=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next, p_temp_edge->p_twin);
				p_temp_edge = p_temp_edge->p_next;
			} while(p_temp_edge != p_face->p_edges);

			struct he_vertex* p_temp_vertex = p_face->p_qh_conflict_list;
			while (p_temp_vertex) {
				log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
				p_temp_vertex = p_temp_vertex->p_next;
			}
		}

		// CHECK FOR TOPOLOGICAL ERRORS

		if (p_edge->p_twin->p_prev->p_twin->p_face == p_edge->p_next->p_twin->p_face) {
			if (p_edge->p_next->p_twin->p_prev == p_edge->p_twin->p_prev->p_twin->p_next) {
				log_msg(LOG_DEBUG, "quickhull", "Topological error! Ingoing and outgoing merge edges pointing to same face (%p) (error 1)\n", p_edge->p_next->p_twin->p_face);
				struct he_edge* p_outer_edge = p_edge->p_next->p_twin->p_prev;

				struct he_edge* p_outer_edge_prev_old = p_outer_edge->p_prev;
				struct he_edge* p_outer_edge_next_old = p_outer_edge->p_next;

				qh_join_edges(p_outer_edge, p_edge->p_twin->p_prev->p_prev);
				qh_join_edges(p_edge->p_next->p_next, p_outer_edge);

				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_outer_edge_prev_old->p_tail);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_prev_old->p_twin);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_prev_old);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_next_old->p_twin);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_next_old);

				struct he_vertex** pp_conflict_list_back = &p_face->p_qh_conflict_list;
				while (*pp_conflict_list_back) {
					pp_conflict_list_back = &(*pp_conflict_list_back)->p_next;
				}
				*pp_conflict_list_back = p_outer_edge->p_face->p_qh_conflict_list;

				// {
				// 	if (p_outer_edge->p_face->p_prev) {
				// 		p_outer_edge->p_face->p_prev->p_next = p_outer_edge->p_face->p_next;
				// 	} else {
				// 		p_hull->p_faces = p_outer_edge->p_face->p_next;
				// 	}

				// 	if (p_outer_edge->p_face->p_next) {
				// 		p_outer_edge->p_face->p_next->p_prev = p_outer_edge->p_face->p_prev;
				// 	}

				// 	qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_faces, (struct qh_freelist_node*)p_outer_edge->p_face);
				// }

				qh_delete_hull_face(p_hull, p_outer_edge->p_face);

				p_outer_edge->p_face = p_face;
			} else {
				log_msg(LOG_DEBUG, "quickhull", "Topological error! Ingoing and outgoing merge edges pointing to same face (%p) (error 2)\n", p_edge->p_next->p_twin->p_face);
				struct he_edge* p_edge_r1 = p_edge->p_next;
				struct he_edge* p_edge_s1 = p_edge->p_twin->p_prev;
				struct he_edge* p_edge_r2 = p_edge_s1->p_twin;
				struct he_edge* p_edge_s2 = p_edge_r1->p_twin;

				qh_join_edges(p_edge_r1->p_next, p_edge_s1);
				qh_join_edges(p_edge_r2->p_next, p_edge_s2);

				qh_twin_edges(p_edge_s1, p_edge_s2);

				if (p_edge_r1->p_face->p_edges == p_edge_r1) {
					p_edge_r1->p_face->p_edges = p_edge_r1->p_next;
					b_restart = 1;
				}

				if (p_edge_r2->p_face->p_edges == p_edge_r2) {
					p_edge_r2->p_face->p_edges = p_edge_r2->p_next;
				}

				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_edge_r1->p_tail);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge_r1);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge_r2);
			}

			{
				log_msg(LOG_DEBUG, "quickhull", "\tFACE POST-TOPOLOGICAL ERROR FIX (%p)\n", p_face);
				log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d, p_prev=%p, p_next=%p\n", p_face->b_qh_processed, p_face->p_prev, p_face->p_next);
				struct he_edge* p_temp_edge = p_face->p_edges;
				do {
					log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p, p_twin=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next, p_temp_edge->p_twin);
					p_temp_edge = p_temp_edge->p_next;
				} while(p_temp_edge != p_face->p_edges);

				struct he_vertex* p_temp_vertex = p_face->p_qh_conflict_list;
				while (p_temp_vertex) {
					log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
					p_temp_vertex = p_temp_vertex->p_next;
				}
			}
		}

		if (p_edge2->p_twin->p_prev->p_twin->p_face == p_edge2->p_next->p_twin->p_face) {
			if (p_edge2->p_next->p_twin->p_prev == p_edge2->p_twin->p_prev->p_twin->p_next) {
				log_msg(LOG_DEBUG, "quickhull", "Topological error! Ingoing and outgoing merge edges pointing to same face (%p) (error 1)\n", p_edge2->p_next->p_twin->p_face);
				struct he_edge* p_outer_edge = p_edge2->p_next->p_twin->p_prev;

				struct he_edge* p_outer_edge_prev_old = p_outer_edge->p_prev;
				struct he_edge* p_outer_edge_next_old = p_outer_edge->p_next;
				
				qh_join_edges(p_outer_edge, p_edge2->p_twin->p_prev->p_prev);
				qh_join_edges(p_edge2->p_next->p_next, p_outer_edge);

				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_outer_edge_prev_old->p_tail);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_prev_old->p_twin);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_prev_old);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_next_old->p_twin);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_outer_edge_next_old);

				struct he_vertex** pp_conflict_list_back = &p_face->p_qh_conflict_list;
				while (*pp_conflict_list_back) {
					pp_conflict_list_back = &(*pp_conflict_list_back)->p_next;
				}
				*pp_conflict_list_back = p_outer_edge->p_face->p_qh_conflict_list;

				// {
				// 	if (p_outer_edge->p_face->p_prev) {
				// 		p_outer_edge->p_face->p_prev->p_next = p_outer_edge->p_face->p_next;
				// 	} else {
				// 		p_hull->p_faces = p_outer_edge->p_face->p_next;
				// 	}

				// 	if (p_outer_edge->p_face->p_next) {
				// 		p_outer_edge->p_face->p_next->p_prev = p_outer_edge->p_face->p_prev;
				// 	}

				// 	qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_faces, (struct qh_freelist_node*)p_outer_edge->p_face);
				// }
				
				qh_delete_hull_face(p_hull, p_outer_edge->p_face);

				p_outer_edge->p_face = p_face;
			} else {
				log_msg(LOG_DEBUG, "quickhull", "Topological error! Ingoing and outgoing merge edges pointing to same face (%p) (error 2)\n", p_edge2->p_next->p_twin->p_face);
				struct he_edge* p_edge_r1 = p_edge2->p_next;
				struct he_edge* p_edge_s1 = p_edge2->p_twin->p_prev;
				struct he_edge* p_edge_r2 = p_edge_s1->p_twin;
				struct he_edge* p_edge_s2 = p_edge_r1->p_twin;

				qh_join_edges(p_edge_r1->p_next, p_edge_s1);
				qh_join_edges(p_edge_r2->p_next, p_edge_s2);

				qh_twin_edges(p_edge_s1, p_edge_s2);

				if (p_edge_r1->p_face->p_edges == p_edge_r1) {
					p_edge_r1->p_face->p_edges = p_edge_r1->p_next;
					b_restart = 1;
				}

				if (p_edge_r2->p_face->p_edges == p_edge_r2) {
					p_edge_r2->p_face->p_edges = p_edge_r2->p_next;
				}

				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_vertices, (struct qh_freelist_node*)p_edge_r1->p_tail);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge_r1);
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge_r2);
			}

			{
				log_msg(LOG_DEBUG, "quickhull", "\tFACE POST-TOPOLOGICAL ERROR FIX (%p)\n", p_face);
				log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d, p_prev=%p, p_next=%p\n", p_face->b_qh_processed, p_face->p_prev, p_face->p_next);
				struct he_edge* p_temp_edge = p_face->p_edges;
				do {
					log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p, p_twin=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next, p_temp_edge->p_twin);
					p_temp_edge = p_temp_edge->p_next;
				} while(p_temp_edge != p_face->p_edges);

				struct he_vertex* p_temp_vertex = p_face->p_qh_conflict_list;
				while (p_temp_vertex) {
					log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
					p_temp_vertex = p_temp_vertex->p_next;
				}
			}
		}

		struct he_edge* p_next = p_edge->p_prev->p_next;

		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge);
		qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge2);

		// {
		// 	if (p_face2->p_prev) {
		// 		p_face2->p_prev->p_next = p_face2->p_next;
		// 	} else {
		// 		p_hull->p_faces = p_face2->p_next;
		// 	}

		// 	if (p_face2->p_next) {
		// 		p_face2->p_next->p_prev = p_face2->p_prev;
		// 	}
		// }

		// qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_faces, (struct qh_freelist_node*)p_face2);

		qh_delete_hull_face(p_hull, p_face2);

		if (b_restart) {
			qh_merge_face_edge_faces(p_hull, p_face, threshold);
			break;
		}

		p_edge = p_next;

	} while(p_edge != p_face->p_edges);
}

static void qh_construct_horizon_faces(struct he_mesh* p_hull, struct he_edge* p_horizon_start, struct he_vertex* p_eye) {
	// log_msg(LOG_DEBUG, "quickhull", "Constructing new faces for horizon...\n");

	// log_msg(LOG_DEBUG, "quickhull", "HORIZON EDGES:\n");
	// struct he_edge* p_temp_horizon_edge = p_horizon_start;
	// while(p_temp_horizon_edge) {
	// 	log_msg(LOG_DEBUG, "quickhull", "\tEdge (%p)\n", p_temp_horizon_edge);
	// 	p_temp_horizon_edge = p_temp_horizon_edge->p_qh_horizon_next;
	// }

	struct he_face* p_new_faces = 0;

	struct he_edge* p_horizon_edge = p_horizon_start;
	
	qh_freelist_set_head((struct qh_freelist_node**)&p_new_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, p_horizon_edge->p_tail, p_horizon_edge->p_next->p_tail, p_eye));

	p_new_faces->p_edges->p_twin = p_horizon_edge->p_twin;
	p_horizon_edge->p_twin->p_twin = p_new_faces->p_edges;

	p_horizon_edge = p_horizon_edge->p_qh_horizon_next;

	struct he_face* p_new_faces_back = p_new_faces;

	while (p_horizon_edge) {
		qh_freelist_set_head((struct qh_freelist_node**)&p_new_faces, (struct qh_freelist_node*)qh_construct_new_face(p_hull, p_horizon_edge->p_tail, p_horizon_edge->p_next->p_tail, p_eye));
		
		p_new_faces->p_edges->p_twin = p_horizon_edge->p_twin;
		p_horizon_edge->p_twin->p_twin = p_new_faces->p_edges;
		
		p_new_faces->p_next->p_edges->p_next->p_next->p_twin = p_new_faces->p_edges->p_next;
		p_new_faces->p_edges->p_next->p_twin = p_new_faces->p_next->p_edges->p_next->p_next;
		
		p_horizon_edge = p_horizon_edge->p_qh_horizon_next;
	}

	p_new_faces->p_edges->p_next->p_next->p_twin = p_new_faces_back->p_edges->p_next;
	p_new_faces_back->p_edges->p_next->p_twin = p_new_faces->p_edges->p_next->p_next;

	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		if (p_face->b_qh_processed) {
			qh_distribute_conflict_vertices(p_hull, p_new_faces, p_face->p_qh_conflict_list);

			if (p_face->p_prev) {
				p_face->p_prev->p_next = p_face->p_next;
			} else {
				p_hull->p_faces = p_face->p_next;
			}

			if (p_face->p_next) {
				p_face->p_next->p_prev = p_face->p_prev;
			}
			
			log_msg(LOG_DEBUG, "quickhull", "Old face (%p) removed (face.prev (%p) now points to (%p), face.next (%p) now points to (%p))\n", p_face, p_face->p_prev, p_face->p_prev ? p_face->p_prev->p_next : 0, p_face->p_next, p_face->p_next ? p_face->p_next->p_prev : 0);

			struct he_edge* p_edge = p_face->p_edges;
			do {
				struct he_edge* p_next_edge = p_edge->p_next;
				qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_edges, (struct qh_freelist_node*)p_edge);
				p_edge = p_next_edge;
			} while(p_edge != p_face->p_edges);
			
			struct he_face* p_next = p_face->p_next;
			qh_freelist_set_head((struct qh_freelist_node**)&p_hull->p_free_faces, (struct qh_freelist_node*)p_face);
			
			p_face = p_next;
		} else {
			p_face = p_face->p_next;
		}
	}
	
	p_hull->p_faces->p_prev = p_new_faces_back;
	p_new_faces_back->p_next = p_hull->p_faces;
	
	p_hull->p_faces = p_new_faces;
	
	log_msg(LOG_DEBUG, "quickhull", "MESH FACES (POST-HORIZON-CONSTRUCTION):\n");
	struct he_face* p_temp = p_hull->p_faces;
	while (p_temp) {
		log_msg(LOG_DEBUG, "quickhull", "\tFACE (%p)\n", p_temp);
		log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d, p_prev=%p, p_next=%p\n", p_temp->b_qh_processed, p_temp->p_prev, p_temp->p_next);
		struct he_edge* p_temp_edge = p_temp->p_edges;
		do {
			log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p, p_twin=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next, p_temp_edge->p_twin);
			p_temp_edge = p_temp_edge->p_next;
		} while(p_temp_edge != p_temp->p_edges);

		struct he_vertex* p_temp_vertex = p_temp->p_qh_conflict_list;
		while (p_temp_vertex) {
			log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
			p_temp_vertex = p_temp_vertex->p_next;
		}

		p_temp = p_temp->p_next;
	}
}

static void qh_merge_coplanars(struct he_mesh* p_hull, float threshold) {
	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		qh_merge_face_edge_faces(p_hull, p_face, threshold);
		p_face = p_face->p_next;
	}
	
	log_msg(LOG_DEBUG, "quickhull", "MESH FACES (POST-MERGE):\n");
	struct he_face* p_temp = p_hull->p_faces;
	while (p_temp) {
		log_msg(LOG_DEBUG, "quickhull", "\tFACE (%p)\n", p_temp);
		log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d\n", p_temp->b_qh_processed);
		struct he_edge* p_temp_edge = p_temp->p_edges;
		do {
			log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next);
			p_temp_edge = p_temp_edge->p_next;
		} while(p_temp_edge != p_temp->p_edges);

		struct he_vertex* p_temp_vertex = p_temp->p_qh_conflict_list;
		while (p_temp_vertex) {
			log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
			p_temp_vertex = p_temp_vertex->p_next;
		}

		p_temp = p_temp->p_next;
	}
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

static void qh_purge_duplicate_input_points(float* p_point_cloud, size_t* p_num_points) {
	qsort(p_point_cloud, *p_num_points, sizeof(float) * 3, (void*)qh_sort_cmp);

	// for (size_t i = 0; i < *p_num_points; ++i) {
	// 	log_msg(LOG_DEBUG, "quicksort", "PRINT SORTED VERTS: [%d]: [%f, %f, %f]\n", i, p_point_cloud[i * 3], p_point_cloud[i * 3 + 1], p_point_cloud[i * 3 + 2]);
	// }

	size_t m = 0;

    for (size_t i = 0; i < *p_num_points; ++i)
    {
		float* p_a = &p_point_cloud[i * 3];
		float* p_b = &p_point_cloud[(m - 1) * 3];
        if (i == 0 || !vec3_cmp(p_a, p_b))
        {
            if (i != m)
            {
				vec3_set(p_a, &p_point_cloud[m * 3]);
            }
            ++m;
        }
    }
  
    *p_num_points = m;
	
	// for (size_t i = 0; i < *p_num_points; ++i) {
	// 	log_msg(LOG_DEBUG, "quicksort", "PRINT PURGED VERTS: [%d]: [%f, %f, %f]\n", i, p_point_cloud[i * 3], p_point_cloud[i * 3 + 1], p_point_cloud[i * 3 + 2]);
	// }
}

void quickhull(float* p_point_cloud, size_t num_points, struct he_mesh* p_hull) {
	qh_purge_duplicate_input_points(p_point_cloud, &num_points);
	
	const size_t max_edges = (3 * num_points - 6) * 4;
	const size_t max_faces = (2 * num_points - 4) * 2;
	const size_t size_chunk0 = sizeof(struct he_vertex) * num_points;
	const size_t size_chunk1 = sizeof(struct he_edge) * max_edges;
	const size_t size_chunk2 = sizeof(struct he_face) * max_faces;
	
	log_msg(LOG_DEBUG, "quickhull", "Quickhull: num_points=%d, max_edges=%d, max_faces=%d\n", num_points, max_edges, max_faces);

	*p_hull = (struct he_mesh){0};

	p_hull->p_buffer = calloc(1, size_chunk0 + size_chunk1 + size_chunk2);

	p_hull->p_free_vertices = p_hull->p_buffer;
	p_hull->p_free_edges = (void*)((char*)p_hull->p_free_vertices + size_chunk0);
	p_hull->p_free_faces = (void*)((char*)p_hull->p_free_edges + size_chunk1);

	qh_freelist_init((struct qh_freelist_node*)p_hull->p_free_vertices, sizeof(*p_hull->p_free_vertices), num_points);
	qh_freelist_init((struct qh_freelist_node*)p_hull->p_free_edges, sizeof(*p_hull->p_free_edges), max_edges);
	qh_freelist_init((struct qh_freelist_node*)p_hull->p_free_faces, sizeof(*p_hull->p_free_faces), max_faces);

	struct he_vertex* initial_hull_vertices[4];

	qh_find_initial_hull_vertices(p_point_cloud, num_points, p_hull, initial_hull_vertices);

	const int b_is_acb = qh_is_facing_vertex(initial_hull_vertices[0], initial_hull_vertices[1], initial_hull_vertices[2], initial_hull_vertices[3]);
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

	qh_distribute_conflict_vertices(p_hull, p_hull->p_faces, p_hull->p_free_vertices);

	{
		log_msg(LOG_DEBUG, "quickhull", "INITIAL MESH FACES:\n");
		struct he_face* p_temp = p_hull->p_faces;
		while (p_temp) {
			log_msg(LOG_DEBUG, "quickhull", "\tFACE (%p)\n", p_temp);
			log_msg(LOG_DEBUG, "quickhull", "\t\tProcessed: %d, p_prev=%p, p_next=%p\n", p_temp->b_qh_processed, p_temp->p_prev, p_temp->p_next);
			struct he_edge* p_temp_edge = p_temp->p_edges;
			do {
			log_msg(LOG_DEBUG, "quickhull", "\t\tHalf-edge (%p) tail=[%f, %f, %f], head=[%f, %f, %f], p_face=%p, p_prev=%p, p_next=%p\n", p_temp_edge, p_temp_edge->p_tail->position[0],  p_temp_edge->p_tail->position[1],  p_temp_edge->p_tail->position[2],  p_temp_edge->p_next->p_tail->position[0],  p_temp_edge->p_next->p_tail->position[1],  p_temp_edge->p_next->p_tail->position[2], p_temp_edge->p_face, p_temp_edge->p_prev, p_temp_edge->p_next);
				p_temp_edge = p_temp_edge->p_next;
			} while(p_temp_edge != p_temp->p_edges);

			struct he_vertex* p_temp_vertex = p_temp->p_qh_conflict_list;
			while (p_temp_vertex) {
				log_msg(LOG_DEBUG, "quickhull", "\t\tConflict Vertex (%p) [%f, %f, %f]\n", p_temp_vertex, p_temp_vertex->position[0], p_temp_vertex->position[1], p_temp_vertex->position[2]);
				p_temp_vertex = p_temp_vertex->p_next;
			}

			p_temp = p_temp->p_next;
		}
	}

	size_t DEBUG_iters = 0;
	size_t DEBUG_MAX_ITERS = 10;
	
	struct he_face* p_face = p_hull->p_faces;
	while (p_face) {
		if (!p_face->p_qh_conflict_list) {
			p_face = p_face->p_next;
			continue;
		}
		
		// if (DEBUG_iters == DEBUG_MAX_ITERS) {
		// 	break;
		// }
		// DEBUG_iters++;

		struct he_vertex* p_eye = qh_next_conflict_vertex(p_face);

		// log_msg(LOG_DEBUG, "quickhull", "Finding horizon for vertex [%f, %f, %f] (%p)\n", p_eye->position[0], p_eye->position[1], p_eye->position[2], p_eye);

		struct he_edge* p_horizon_start = 0;
		qh_find_horizon(p_face->p_edges, p_eye, &p_horizon_start);
		
		qh_construct_horizon_faces(p_hull, p_horizon_start, p_eye);

		qh_merge_coplanars(p_hull, 0.001f);

		p_face = p_hull->p_faces;
	}
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