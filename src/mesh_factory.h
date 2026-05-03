#ifndef _H__MESH_FACTORY
#define _H__MESH_FACTORY

struct he_mesh;
struct mesh_primitive;

void mesh_factory_make_quad(const float p_half_size[3], struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_box(const float p_hald_size[3], struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_sphere(
	const float radius,
	const unsigned int n,
	struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_hemisphere(
	const float radius,
	const unsigned int rings, const unsigned int segments,
	struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_cylinder(
	const float radius_a, const float radius_b, const float half_length,
	const unsigned int segments,
	const int b_cap_a, const int b_cap_b,
	struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_from_halfedge_mesh(
	const struct he_mesh* p_he_mesh,
	struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_free_primitive(struct mesh_primitive* p_out_mesh_primitive);

#endif
