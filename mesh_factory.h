#ifndef _H__MESH_FACTORY
#define _H__MESH_FACTORY

#include <stdint.h>

struct he_mesh;
struct mesh_primitive;

void mesh_factory_make_plane(float x, float y, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_box(float x, float y, float z, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_uv_sphere_primitive(float r, size_t n, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_from_halfedge_mesh(const struct he_mesh* p_he_mesh, struct mesh_primitive* p_mesh_primitive, int b_lines);
void mesh_factory_free_primitive(struct mesh_primitive* p_mesh_primitive);

#endif