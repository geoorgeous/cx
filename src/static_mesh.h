#ifndef STATIC_MESH_H
#define STATIC_MESH_H

#include <stdint.h>

#include "cx_asset.h"

#define ASSET_TYPE_STATIC_MESH 4

struct cx_gfx_mesh;
struct cx_mesh_data;

struct static_mesh {
	struct cx_mesh_data* p_primitives;
	size_t num_primitives;
	cx_asset_handle* p_materials;
	struct cx_gfx_mesh* p_gfx_meshes;
	int b_loaded_device_meshes;
};

void static_mesh_free(struct static_mesh* p_static_mesh);
void static_mesh_load_device_meshes(struct static_mesh* p_static_mesh);
void static_mesh_unload_device_meshes(struct static_mesh* p_static_mesh);

void cx_asset_free_static_mesh(void* p);

#endif
