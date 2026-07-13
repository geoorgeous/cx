#ifndef MATERIAL_H
#define MATERIAL_H

#include "cx_asset.h"

#define ASSET_TYPE_MATERIAL 3

struct material {
	cx_asset_handle p_texture;
	float        color[4];
};

struct cx_stream_writer;
struct cx_stream_reader;

int material_serialize(const struct material* p_material, struct cx_stream_writer* p_writer);
int material_deserialize(struct material* p_material, struct cx_stream_reader* p_reader);

static inline int material_asset_serialize(struct cx_stream_writer* p_writer, const void* p) {
	return material_serialize(p, p_writer);
}

static inline int material_asset_deserialize(struct cx_stream_reader* p_reader, void* p) {
	return material_deserialize(p, p_reader);
}

#endif
