#include "cx_app.h"
#include "cx_asset_package.h"
#include "cx_asset_package_registry.h"
#include "cx_ed.h"
#include "cx_ed_asset_package_builder.h"
#include "cx_ed_import_bdf.h"
#include "cx_ed_import_gltf.h"

static struct cx_asset_package built_in_assets_pkg;

static int cx_editor_init(void);
static void cx_editor_update(double);
static void cx_editor_draw(const struct cx_gfx_framebuffer*);
static void cx_editor_shutdown(void);

static void cx_editor_rebuild_built_in_assets_package(void);

int cx_editor_init(void) {
	cx_asset_package_load_records_from_file(&built_in_assets_pkg, "res/builtin.cxpkg");
	cx_asset_package_registry_mount(&built_in_assets_pkg);

	cx_ed_init(cx_app_primary_window());
	return 0;
}

void cx_editor_update(double frame_delta_time) {
	cx_ed_update(frame_delta_time);
}

void cx_editor_draw(const struct cx_gfx_framebuffer* p_frambuffer) {
	cx_ed_draw(p_frambuffer, 1920, 1080);
}

void cx_editor_shutdown(void) {
	cx_ed_shutdown();
}

void cx_editor_rebuild_built_in_assets_package(void) {
	struct cx_asset_package package;

	cx_asset_handle handle;

	cx_ed_import_gltf_file("res/builtin/gizmo_translate.glb", &handle);
	cx_ed_asset_package_builder_add_asset(&package, handle);

	cx_ed_import_gltf_file("res/builtin/gizmo_rotate.glb", &handle);
	cx_ed_asset_package_builder_add_asset(&package, handle);

	cx_ed_import_gltf_file("res/builtin/gizmo_scale.glb", &handle);
	cx_ed_asset_package_builder_add_asset(&package, handle);

	cx_ed_import_bdf_file("res/builtin/font_dbg_8x14.bdf", &handle);
	cx_ed_asset_package_builder_add_asset(&package, handle);

	cx_ed_asset_package_builder_export(&package, "res/builtin.cxpkg");
}

int main(int argc, const char** argv) {
	(void)argc;
	(void)argv;

	cx_app_init("cx editor", 1920, 1080, cx_editor_init);
	cx_app_run(cx_editor_update, cx_editor_draw);
	cx_app_shutdown(cx_editor_shutdown);
}
