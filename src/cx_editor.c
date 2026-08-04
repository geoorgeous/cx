#include <stdio.h>

#include "cx_app.h"
#include "cx_asset_cache.h"
#include "cx_asset_package.h"
#include "cx_command.h"
#include "cx_command_registry.h"
#include "cx_console.h"
#include "cx_ed.h"
#include "cx_ed_asset_library.h"
#include "cx_ed_asset_package_builder.h"
#include "cx_ed_import_bdf.h"
#include "cx_ed_import_gltf.h"
#include "cx_macro.h"

static struct cx_asset_package built_in_assets_pkg;

static int cx_editor_init(void);
static void cx_editor_update(double);
static void cx_editor_draw(const struct cx_gfx_framebuffer*);
static void cx_editor_shutdown(void);

static void cx_editor_rebuild_built_in_assets_package(void);

static int cx_editor_rebuild_built_in_assets_package_commands(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static int cx_asset_source_deserialize_library_asset(cx_asset_id id, void* p_context, void* p_out) {
	(void)p_context;
	return cx_ed_asset_library_deserialize_asset(id, p_out);
}

static int cx_asset_source_deserialize_package_asset(cx_asset_id id, void* p_context, void* p_out) {
	const struct cx_asset_package* p_package = p_context;
	return cx_asset_package_deserialize_asset(p_package, id, p_out);
}

int cx_editor_init(void) {
	cx_asset_cache_push_source(&(struct cx_asset_source) {
		.f_try_deserialize_asset = cx_asset_source_deserialize_library_asset
	});

	// todo: remove
	cx_editor_rebuild_built_in_assets_package();

	cx_asset_package_import("res/builtin/builtin.cxpkg", &built_in_assets_pkg);
	cx_asset_cache_push_source(&(struct cx_asset_source) {
		.p_context = &built_in_assets_pkg,
		.f_try_deserialize_asset = cx_asset_source_deserialize_package_asset
	});

	CX_NEW_COMMAND(
		"rebuild_built_in_asset_package",
		CX_NULL,
		cx_editor_rebuild_built_in_assets_package_commands,
		CX_NULL,
		CX_COMMAND_NO_PARAMS);

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
	cx_ed_asset_library_free();
}

void cx_editor_rebuild_built_in_assets_package(void) {
	struct cx_ed_asset_package_builder package_builder = {0};

	struct cx_asset_ref asset_ref;

	FILE* p_file = fopen("src/cx_ed_builtin_asset_ids.h", "w");

	fputs("#ifndef CX_ED_BUILTIN_ASSET_IDS_H\n", p_file);
	fputs("#define CX_ED_BUILTIN_ASSET_IDS_H\n\n", p_file);
	fputs("/* Generated file. Do not edit. */\n\n", p_file);

	cx_ed_import_bdf_file("res/builtin/font_dbg_8x14.bdf", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);
	fprintf(p_file, "#define CX_ED_BUILTIN_ASSET_ID_FONT_DEFAULT %u\n", asset_ref.asset_id);

	cx_ed_import_gltf_file("res/builtin/gizmo_translate.glb", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);
	fprintf(p_file, "#define CX_ED_BUILTIN_ASSET_ID_BLUEPRINT_GIZMO_TRANSLATE %u\n", asset_ref.asset_id);

	cx_ed_import_gltf_file("res/builtin/gizmo_rotate.glb", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);
	fprintf(p_file, "#define CX_ED_BUILTIN_ASSET_ID_BLUEPRINT_GIZMO_ROTATE %u\n", asset_ref.asset_id);

	cx_ed_import_gltf_file("res/builtin/gizmo_scale.glb", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);
	fprintf(p_file, "#define CX_ED_BUILTIN_ASSET_ID_BLUEPRINT_GIZMO_SCALE %u\n", asset_ref.asset_id);

	fputs("\n#endif", p_file);

	fclose(p_file);

	cx_ed_asset_package_builder_export(&package_builder, "res/builtin/builtin.cxpkg");

	cx_ed_asset_package_builder_free(&package_builder);
}

int cx_editor_rebuild_built_in_assets_package_commands(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_args;
	(void)p_context;
	cx_editor_rebuild_built_in_assets_package();
	return 0;
}

int main(int argc, const char** argv) {
	(void)argc;
	(void)argv;

	cx_app_init("cx editor", 1920, 1080, cx_editor_init);
	cx_app_run(cx_editor_update, cx_editor_draw);
	cx_app_shutdown(cx_editor_shutdown);
}
