#include "cx_app.h"
#include "cx_asset_cache.h"
#include "cx_command.h"
#include "cx_console.h"
#include "cx_ed.h"
#include "cx_ed_asset_library.h"
#include "cx_ed_asset_package_builder.h"
#include "cx_ed_import_bdf.h"
#include "cx_ed_import_gltf.h"
#include "cx_macro.h"

static int cx_editor_init(int argc, const char** argv);
static void cx_editor_update(double);
static void cx_editor_draw(const struct cx_gfx_framebuffer*);
static void cx_editor_shutdown(void);

static void cx_editor_rebuild_core_asset_package(void);

static int cx_editor_rebuild_core_asset_package_command(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static int cx_asset_source_get_library_asset_name(cx_asset_id id, void* p_context, const char** pp_out);
static int cx_asset_source_find_library_asset_by_name(
	cx_asset_type type, const char* s_name, void* p_context, struct cx_asset_ref* p_out_ref);
static int cx_asset_source_deserialize_library_asset(cx_asset_id id, void* p_context, void* p_out);

int cx_editor_init(int argc, const char** argv) {
	(void)argc;
	(void)argv;

	cx_asset_cache_push_source(&(struct cx_asset_source) {
		.f_get_asset_name = cx_asset_source_get_library_asset_name,
		.f_find_asset_by_name = cx_asset_source_find_library_asset_by_name,
		.f_try_deserialize_asset = cx_asset_source_deserialize_library_asset
	});

	//cx_editor_rebuild_core_asset_package();

	CX_NEW_CONSOLE_COMMAND(
		"rebuild_core_pkg",
		"",
		cx_editor_rebuild_core_asset_package_command,
		CX_NULL,
		CX_CONSOLE_COMMAND_NO_PARAMS);

	//cx_ed_init(cx_app_primary_window());

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

void cx_editor_rebuild_core_asset_package(void) {
	struct cx_ed_asset_package_builder package_builder = {0};

	struct cx_asset_ref asset_ref;

	cx_ed_import_bdf_file("res/builtin/default_8x14.bdf", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

	cx_ed_import_gltf_file("res/builtin/gizmo_translate.glb", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

	cx_ed_import_gltf_file("res/builtin/gizmo_rotate.glb", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

	cx_ed_import_gltf_file("res/builtin/gizmo_scale.glb", &asset_ref);
	cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

	cx_ed_asset_package_builder_export(&package_builder, "res/builtin/core.cxpkg");

	cx_ed_asset_package_builder_free(&package_builder);
}

int cx_asset_source_get_library_asset_name(cx_asset_id id, void* p_context, const char** pp_out) {
	(void)p_context;
	return cx_ed_asset_library_get_asset_name(id, pp_out);
}

int cx_asset_source_find_library_asset_by_name(cx_asset_type type, const char* s_name, void* p_context, struct cx_asset_ref* p_out_ref) {
	(void)p_context;
	return cx_ed_asset_library_find_asset_by_name(type, s_name, p_out_ref);
}

int cx_asset_source_deserialize_library_asset(cx_asset_id id, void* p_context, void* p_out) {
	(void)p_context;
	return cx_ed_asset_library_deserialize_asset(id, p_out);
}

int cx_editor_rebuild_core_asset_package_command(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_args;
	(void)p_context;
	cx_editor_rebuild_core_asset_package();
	return 0;
}

int main(int argc, const char** argv) {
	(void)argc;
	(void)argv;

	cx_app_init("cx editor", 1920, 1080, cx_editor_init);
	cx_app_run(cx_editor_update, cx_editor_draw);
	cx_app_shutdown(cx_editor_shutdown);
}
