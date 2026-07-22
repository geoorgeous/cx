#ifndef MATERIAL_H
#define MATERIAL_H

#include "cx_asset.h"

#define CX_ASSET_TYPE_MATERIAL 3

struct material {
	cx_asset_handle p_texture;
	float        color[4];
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

#endif
