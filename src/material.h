#ifndef MATERIAL_H
#define MATERIAL_H

#include "cx_asset_defs.h"

#define CX_ASSET_TYPE_MATERIAL 3

struct material {
	struct cx_asset_ref texture_asset_ref;
	float color[4];
};

struct cx_stream;

int material_serialize(const struct material* p_material, struct cx_stream* p_stream);
int material_deserialize(struct cx_stream* p_stream, struct material* p_material);

static inline int material_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return material_serialize(p_asset, p_stream);
}

static inline int material_asset_deserialize(struct cx_stream* p_stream, void* p_out_asset) {
	return material_deserialize(p_stream, p_out_asset);
}

static inline void material_asset_enumerate_dependencies(
	const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr) {
	
	const struct material* p_material = p_asset;
	f_cb(p_material->texture_asset_ref.asset_id, p_user_ptr);
}

#endif
