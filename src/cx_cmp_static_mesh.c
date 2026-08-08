#include "cx_asset.h"
#include "cx_cmp_static_mesh.h"
#include "cx_macro.h"

struct cx_component_type cmp_type_static_mesh = {
	.s_name = "static_mesh",
	.f_serialize = cx_cmp_static_mesh_serialize,
	.f_deserialize = cx_cmp_static_mesh_deserialize,
	.f_enumerate_asset_dependencies = cx_cmp_static_mesh_enumerate_asset_dependencies,
	.size = sizeof(struct cx_cmp_static_mesh),
	.alignment = CX_ALIGNOF(struct cx_cmp_static_mesh)
};

int cx_cmp_static_mesh_serialize(const void* p_cmp, struct cx_stream* p_stream) {
	const struct cx_cmp_static_mesh* p_cmp_static_mesh = p_cmp;

	cx_asset_ref_serialize(&p_cmp_static_mesh->asset_ref, p_stream);

	return CX_TRUE;
}

int cx_cmp_static_mesh_deserialize(struct cx_stream* p_stream, void* p_out_cmp) {
	struct cx_cmp_static_mesh* p_out_cmp_static_mesh = p_out_cmp;

	cx_asset_ref_deserialize(p_stream, &p_out_cmp_static_mesh->asset_ref);

	return CX_TRUE;
}

void cx_cmp_static_mesh_enumerate_asset_dependencies(
	const void* p_cmp, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr) {
	
	const struct cx_cmp_static_mesh* p_cmp_static_mesh = p_cmp;

	f_cb(p_cmp_static_mesh->asset_ref.asset_id, p_user_ptr);
}
