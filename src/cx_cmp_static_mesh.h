#ifndef CX_CMP_STATIC_MESH_H
#define CX_CMP_STATIC_MESH_H

#include "cx_asset.h"
#include "cx_component.h"

extern struct cx_component_type cmp_type_static_mesh;

struct cx_cmp_static_mesh {
	struct cx_asset_ref asset_ref;
};

int cx_cmp_static_mesh_serialize(const void* p_cmp, struct cx_stream* p_stream);
int cx_cmp_static_mesh_deserialize(struct cx_stream* p_stream, void* p_out_cmp);

#endif
