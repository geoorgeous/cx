#ifndef _H__QUICKHULL
#define _H__QUICKHULL

#include <stdint.h>

#include "half_edge.h"
#include "static_mesh.h"

struct static_mesh;

void quickhull(float* p_vertices, size_t num_vertices, struct he_mesh* p_result);
void quickhull_static_mesh(const struct static_mesh* p_static_mesh, struct he_mesh* p_result);
void quickhull_free(struct he_mesh* p_mesh);

#endif