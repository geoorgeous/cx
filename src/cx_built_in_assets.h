#ifndef _H__CX_BUILT_IN_ASSETS
#define _H__CX_BUILT_IN_ASSETS

enum cx_built_in_asset {
	CX_BUILT_IN_ASSET_mesh_sphere,
	CX_BUILT_IN_ASSET_mesh_capsule_cap,
	CX_BUILT_IN_ASSET_mesh_capsule_mid,
	CX_BUILT_IN_ASSET_mesh_cube,
	CX_BUILT_IN_ASSET_mesh_plane,
};

void* cx_built_in_assets_get(enum cx_built_in_asset built_in_asset);

#endif