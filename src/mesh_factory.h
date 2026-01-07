#ifndef _H__MESH_FACTORY
#define _H__MESH_FACTORY

#include <stddef.h>

struct he_mesh;
struct mesh_primitive;

void mesh_factory_make_quad(const float* p_half_size_xz, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_box(const float* p_hald_size_xyz, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_sphere(float r, size_t n, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_hemisphere(float r, size_t n, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_cylinder(float r0, float r1, float y, size_t n, int b_cap0, int b_cap1, struct mesh_primitive* p_mesh_primitive);
void mesh_factory_make_from_halfedge_mesh(const struct he_mesh* p_he_mesh, struct mesh_primitive* p_mesh_primitive, int b_lines);
void mesh_factory_free_primitive(struct mesh_primitive* p_mesh_primitive);

#endif
