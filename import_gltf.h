#ifndef _H__IMPORT_GLTF
#define _H__IMPORT_GLTF

#include <stdint.h>

struct asset_package;
struct gltf;

struct import_gltf_result {
    asset_handle* p_images;
    size_t        num_images;
    asset_handle* p_textures;
    size_t        num_textures;
    asset_handle* p_materials;
    size_t        num_materials;
    asset_handle* p_meshes;
    size_t        num_meshes;
    asset_handle* p_skeletons;
    size_t        num_skeleton;
    asset_handle* p_scenes;
    size_t        num_scenes;
};

void import_gltf(const struct gltf* p_gltf, struct asset_package* p_asset_package, struct import_gltf_result* p_result);
void import_gltf_free(struct import_gltf_result* p_result);

#endif