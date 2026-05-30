#include <stdlib.h>

#include "cx_gfx_mesh.h"
#include "mesh.h"
#include "static_mesh.h"

void static_mesh_free(struct static_mesh* p_static_mesh) {
	for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
		for (size_t j = 0; j < p_static_mesh->p_primitives[i].num_vertex_buffers; ++j) {
			free(p_static_mesh->p_primitives[i].p_vertex_buffers[j].p_bytes);
		}

		free(p_static_mesh->p_primitives[i].p_vertex_buffers);
		free(p_static_mesh->p_primitives[i].index_buffer.p_bytes);
		free(p_static_mesh->p_primitives[i].p_attributes);
	}

	free(p_static_mesh->p_primitives);
	free(p_static_mesh->p_materials);

    static_mesh_unload_device_meshes(p_static_mesh);
    *p_static_mesh = (struct static_mesh){0};
}

void static_mesh_load_device_meshes(struct static_mesh* p_static_mesh) {
    p_static_mesh->p_gfx_meshes = malloc(sizeof(*p_static_mesh->p_gfx_meshes) * p_static_mesh->num_primitives);

    for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
        const struct mesh_primitive* p_primitive = &p_static_mesh->p_primitives[i];
        cx_gfx_mesh_create(&p_static_mesh->p_gfx_meshes[i], p_primitive);
    }

    p_static_mesh->b_loaded_device_meshes = 1;
}

void static_mesh_unload_device_meshes(struct static_mesh* p_static_mesh) {
    for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
        cx_gfx_mesh_destroy(&p_static_mesh->p_gfx_meshes[i]);
    }
    free(p_static_mesh->p_gfx_meshes);
    p_static_mesh->b_loaded_device_meshes = 0;
}

void cx_asset_free_static_mesh(void* p) {
	static_mesh_free(p);
}
