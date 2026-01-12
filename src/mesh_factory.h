#ifndef _H__MESH_FACTORY
#define _H__MESH_FACTORY

struct he_mesh;
struct mesh_primitive;

void mesh_factory_make_quad(const float* p_half_size, struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_box(const float* p_hald_size, struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_sphere(float radius, unsigned int n, struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_hemisphere(float radius, unsigned int n, struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_cylinder(
	float radius_a, float radius_b,
	float half_length,
	unsigned int n,
	int b_cap_a, int b_cap_b,
	struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_make_from_halfedge_mesh(
	const struct he_mesh* p_he_mesh,
	struct mesh_primitive* p_out_mesh_primitive);

void mesh_factory_free_primitive(struct mesh_primitive* p_out_mesh_primitive);

#endif
