#ifndef CX_MESH_GEN_H
#define CX_MESH_GEN_H

struct he_mesh;
struct mesh_primitive;

void cx_mesh_gen_quad(
	float half_width,
	float half_height,
	const float* p_normal,
	struct mesh_primitive* p_out_mesh_primitive);

void cx_mesh_gen_box(const float p_hald_size[3], struct mesh_primitive* p_out_mesh_primitive);

void cx_mesh_gen_sphere(
	const float radius,
	const unsigned int rings, const unsigned int segments,
	struct mesh_primitive* p_out_mesh_primitive);

void cx_mesh_gen_hemisphere(
	const float radius,
	const unsigned int rings, const unsigned int segments,
	struct mesh_primitive* p_out_mesh_primitive);

void cx_mesh_gen_cylinder(
	const float radius_a, const float radius_b, const float half_length,
	const unsigned int segments,
	const int b_cap_a, const int b_cap_b,
	struct mesh_primitive* p_out_mesh_primitive);

void cx_mesh_gen_from_halfedge_mesh(
	const struct he_mesh* p_he_mesh,
	struct mesh_primitive* p_out_mesh_primitive);

void cx_mesh_gen_free(struct mesh_primitive* p_out_mesh_primitive);

#endif
