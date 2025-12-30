#include "asset.h"
#include "cx_built_in_assets.h"
#include "gl_mesh.h"
#include "mesh_factory.h"
#include "mesh.h"
#include "static_mesh.h"

static struct gl_mesh g_gl_mesh_sphere;
static struct gl_mesh g_gl_mesh_capsule_cap;
static struct gl_mesh g_gl_mesh_capsule_mid;
static struct gl_mesh g_gl_mesh_box;
static struct gl_mesh g_gl_mesh_plane;

static void* g_built_in_assets[5];

static void cx_built_in_asset_init(void);

void* cx_built_in_assets_get(enum cx_built_in_asset built_in_asset) {
	cx_built_in_asset_init();
	return g_built_in_assets[built_in_asset];
}

void cx_built_in_asset_init(void) {
	static int b_initialized = 0;

	if (b_initialized) {
		return;
	}

	struct mesh_primitive mesh_prim;

	mesh_factory_make_uv_sphere_primitive(0.5f, 12, &mesh_prim);
	gl_mesh_create(&g_gl_mesh_sphere, &mesh_prim);
	mesh_factory_free_primitive(&mesh_prim);
	g_built_in_assets[0] = &g_gl_mesh_sphere;

	// todo: hemisphere
	mesh_factory_make_uv_sphere_primitive(0.5f, 12, &mesh_prim);
	gl_mesh_create(&g_gl_mesh_sphere, &mesh_prim);
	mesh_factory_free_primitive(&mesh_prim);
	g_built_in_assets[1] = &g_gl_mesh_capsule_cap;
	
	// todo: cylinder, no top or bottom
	mesh_factory_make_uv_sphere_primitive(0.5f, 12, &mesh_prim);
	gl_mesh_create(&g_gl_mesh_sphere, &mesh_prim);
	mesh_factory_free_primitive(&mesh_prim);
	g_built_in_assets[2] = &g_gl_mesh_capsule_mid;

	mesh_factory_make_box(1, 1, 1, &mesh_prim);
	gl_mesh_create(&g_gl_mesh_sphere, &mesh_prim);
	mesh_factory_free_primitive(&mesh_prim);
	g_built_in_assets[3] = &g_gl_mesh_box;

	mesh_factory_make_plane(1, 1, &mesh_prim);
	gl_mesh_create(&g_gl_mesh_sphere, &mesh_prim);
	mesh_factory_free_primitive(&mesh_prim);
	g_built_in_assets[4] = &g_gl_mesh_plane;

	b_initialized = 1;
}