#include "cx_cmp_static_mesh.h"
#include "cx_asset.h"

struct cx_component_type cmp_type_static_mesh = {
	.s_name = "static_mesh",
	.f_serialize = cx_cmp_static_mesh_serialize,
	.f_deserialize = cx_cmp_static_mesh_deserialize,
	.size = sizeof(struct cx_cmp_static_mesh),
	.alignment = CX_ALIGNOF(struct cx_cmp_static_mesh)
};

int cx_cmp_static_mesh_serialize(const void* p_cmp, struct cx_stream* p_stream) {
	const struct cx_cmp_static_mesh* p_cmp_static_mesh = p_cmp;
	return cx_asset_handle_serialize(p_cmp_static_mesh->p_asset_package_record, p_stream);
}

int cx_cmp_static_mesh_deserialize(struct cx_stream* p_stream, void* p_out_cmp) {
	struct cx_cmp_static_mesh* p_out_cmp_static_mesh = p_out_cmp;
	return cx_asset_handle_deserialize(p_stream, &p_out_cmp_static_mesh->p_asset_package_record);
}
