#ifndef STATIC_MESH_H
#define STATIC_MESH_H

#include <stdint.h>

#include "cx_asset.h"

#define ASSET_TYPE_STATIC_MESH 4

struct cx_gfx_mesh;
struct cx_mesh_data;

struct static_mesh {
	struct cx_mesh_data* p_primitives;
	uint16_t num_primitives;
	cx_asset_handle* p_materials;
	struct cx_gfx_mesh* p_gfx_meshes;
	int b_loaded_device_meshes;
};

void static_mesh_free(struct static_mesh* p_static_mesh);
void static_mesh_load_device_meshes(struct static_mesh* p_static_mesh);
void static_mesh_unload_device_meshes(struct static_mesh* p_static_mesh);
int static_mesh_serialize(const struct static_mesh* p_static_mesh, struct cx_stream* p_stream);
int static_mesh_deserialize(struct cx_stream* p_stream, struct static_mesh* p_out_static_mesh);

static inline void static_mesh_asset_destroy(void* p_asset) {
	static_mesh_free(p_asset);
}

static inline int static_mesh_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return static_mesh_serialize(p_asset, p_stream);
}

static inline int static_mesh_asset_deserialize(struct cx_stream* p_stream, void* p_out_asset) {
	return static_mesh_deserialize(p_stream, p_out_asset);
}

#endif
