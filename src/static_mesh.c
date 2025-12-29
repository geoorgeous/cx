#include <stdlib.h>

#include "gl_mesh.h"
#include "mesh.h"
#include "static_mesh.h"

void static_mesh_free(struct static_mesh* p_static_mesh) {
    free(p_static_mesh->p_gl_meshes);
    static_mesh_unlod_device_meshes(p_static_mesh);
    *p_static_mesh = (struct static_mesh){0};
}

void static_mesh_load_device_meshes(struct static_mesh* p_static_mesh) {
    p_static_mesh->p_gl_meshes = malloc(sizeof(*p_static_mesh->p_gl_meshes) * p_static_mesh->num_primitives);

    for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
        const struct mesh_primitive* p_primitive = &p_static_mesh->p_primitives[i];
        gl_mesh_create(&p_static_mesh->p_gl_meshes[i], p_primitive);
    }

    p_static_mesh->b_loaded_device_meshes = 1;
}

void static_mesh_unlod_device_meshes(struct static_mesh* p_static_mesh) {
    for (size_t i = 0; i < p_static_mesh->num_primitives; ++i) {
        gl_mesh_destroy(&p_static_mesh->p_gl_meshes[i]);
    }
    free(p_static_mesh->p_gl_meshes);
    p_static_mesh->b_loaded_device_meshes = 0;
}