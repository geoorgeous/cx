#ifndef _H__STATIC_MESH
#define _H__STATIC_MESH

#include <stdint.h>

#include "asset.h"

#define ASSET_TYPE_STATIC_MESH 4

struct gl_mesh;
struct mesh_primitive;

struct static_mesh {
    struct mesh_primitive* p_primitives;
    size_t                 num_primitives;
    asset_handle*          p_materials;
    struct gl_mesh*        p_gl_meshes;
    int                    b_loaded_device_meshes;
};

void static_mesh_free(struct static_mesh* p_static_mesh);
void static_mesh_load_device_meshes(struct static_mesh* p_static_mesh);
void static_mesh_unlod_device_meshes(struct static_mesh* p_static_mesh);

#endif