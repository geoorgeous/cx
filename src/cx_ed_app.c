#include <string.h>

#include "cx_app.h"
#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_command.h"
#include "cx_console.h"
#include "cx_ed_asset_library.h"
#include "cx_ed_asset_package_builder.h"
#include "cx_ed_import.h"
#include "cx_ed_import_bdf.h"
#include "cx_ed_import_gltf.h"
#include "cx_ed_world_editor.h"
#include "cx_gfx_render_pass.h"
#include "cx_gfx_shader_program.h"
#include "cx_io.h"
#include "cx_macro.h"
#include "cx_material.h"
#include "cx_shader.h"

// TODO(george): finish material serialization

static int cx_ed_app_init(int argc, const char** argv);
static void cx_ed_app_update(double);
static void cx_ed_app_draw(const struct cx_gfx_framebuffer*);
static void cx_ed_app_shutdown(void);

static int cx_cmd_list_assets(const struct cx_command_args* p_args, const struct cx_command_context* p_context);
static void cx_cmp_list_assets_asset_library_enumerate_cb(
	const struct cx_ed_asset_library_entry* p_entry, void* p_user_ptr);

static int cx_cmd_import(const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static int cx_ed_app_open_world_editor_command(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static int cx_cmd_save_all(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static void cx_ed_rebuild_core_asset_package(void);
static int cx_ed_rebuild_core_asset_package_command(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static int cx_asset_source_get_library_asset_name(cx_asset_id id, void* p_context, const char** pp_out);
static int cx_asset_source_find_library_asset_by_name(
	cx_asset_type type, const char* s_name, void* p_context, struct cx_asset_ref* p_out_ref);
static int cx_asset_source_deserialize_library_asset(cx_asset_id id, void* p_context, void* p_out);

static int b_is_world_editor_open;

int cx_ed_app_init(int argc, const char** argv) {
	(void)argc;
	(void)argv;

	cx_asset_cache_push_source(&(struct cx_asset_source) {
		.f_get_asset_name = cx_asset_source_get_library_asset_name,
		.f_find_asset_by_name = cx_asset_source_find_library_asset_by_name,
		.f_try_deserialize_asset = cx_asset_source_deserialize_library_asset
	});

	cx_ed_rebuild_core_asset_package();
	
	CX_NEW_CONSOLE_COMMAND(
		"cx.listassets", 
		"List all assets in the editor asset library", 
		cx_cmd_list_assets, 
		CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("type", "The asset type to filter by"), OPTIONAL));

	CX_NEW_CONSOLE_COMMAND(
		"cx.import",
		"Import an asset file", 
		cx_cmd_import, 
		CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING("filepath", "Filepath of the asset to import"), REQUIRED));

	CX_NEW_CONSOLE_COMMAND(
		"cx.worldedit", 
		"", 
		cx_ed_app_open_world_editor_command, 
		CX_NULL,
		CX_CONSOLE_COMMAND_PARAM(STRING(
			"name",
			"Name of the world_blueprint asset to open, or the name of the new world_blueprint"),
			REQUIRED)
		);

	CX_NEW_CONSOLE_COMMAND(
		"cx.saveall",
		"",
		cx_cmd_save_all,
		CX_NULL,
		CX_CONSOLE_COMMAND_NO_PARAMS);

	CX_NEW_CONSOLE_COMMAND(
		"cx.rebuildcorepkg",
		"",
		cx_ed_rebuild_core_asset_package_command,
		CX_NULL,
		CX_CONSOLE_COMMAND_NO_PARAMS);

	struct cx_asset_ref ref;

	cx_ed_import_file("res/test.glb", &ref);
	
	//cx_ed_asset_library_scan_dir("./res");
	//cx_ed_asset_library_add_file("./testworld.cxasset", &ref);
	cx_ed_asset_library_enumerate_assets(cx_cmp_list_assets_asset_library_enumerate_cb, CX_NULL);

	cx_ed_world_editor_init(cx_app_primary_window(), "testworld");
	b_is_world_editor_open = CX_TRUE;

	return 0;
}

void cx_ed_app_update(double frame_delta_time) {
	if (b_is_world_editor_open) {
		cx_ed_world_editor_update(frame_delta_time);
	}
}

void cx_ed_app_draw(const struct cx_gfx_framebuffer* p_frambuffer) {
	if (b_is_world_editor_open) {
		cx_ed_world_editor_draw(p_frambuffer, 960, 640);
	}
}

void cx_ed_app_shutdown(void) {
	if (b_is_world_editor_open) {
		cx_ed_world_editor_shutdown();
	}

	cx_ed_asset_library_free();
}

int cx_cmd_list_assets(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	cx_ed_asset_library_enumerate_assets(cx_cmp_list_assets_asset_library_enumerate_cb, p_context->p_flogger);

	// todo filter by type

	return 0;
}

void cx_cmp_list_assets_asset_library_enumerate_cb(const struct cx_ed_asset_library_entry* p_entry, void* p_user_ptr) {
	(void)p_user_ptr;

	CX_LOG_FMT(INFO, ASSET, "[%X:%s] %s\n",
		p_entry->asset_ref.asset_id,
		cx_asset_type_display_name_str(CX_ASSET_GET_TYPE_ID(p_entry->asset_ref.asset_id)),
		p_entry->name);
}

int cx_cmd_import(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_context;

	struct cx_asset_ref asset_ref;
	int b_success = cx_ed_import_file(p_args->list[0].as_str.p, &asset_ref);

	return b_success != CX_TRUE;
}

int cx_ed_app_open_world_editor_command(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	(void)p_context;

	if (b_is_world_editor_open) {
		return 0;
	}

	cx_ed_world_editor_init(cx_app_primary_window(), p_args->list->as_str.p);

	b_is_world_editor_open = CX_TRUE;
	
	return 0;
}

static int cx_cmd_save_all(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {

	(void)p_args;
	(void)p_context;

	cx_ed_asset_library_save_all_unsaved();

	return CX_SUCCESS;
}

void cx_ed_rebuild_core_asset_package(void) {
	struct cx_ed_asset_package_builder package_builder = {0};

	struct cx_asset_ref asset_ref;

	void* p_vertex_stage_file_contents;
	void* p_fragment_stage_file_contents;
	struct cx_shader_source shader_source;
	struct cx_shader* p_shader;
	
	{
		shader_source.p_vertex_stage_source =
				"#version 330 core\n"
				"out vec2 v_texcoords;\n"
				"void main() {\n"
					"vec2 vertices[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));\n"
					"gl_Position = vec4(vertices[gl_VertexID], 0, 1);\n"
					"v_texcoords = 0.5 * gl_Position.xy + vec2(0.5);\n"
				"}";
		shader_source.vertex_stage_source_len = strlen(shader_source.p_vertex_stage_source);

		shader_source.p_fragment_stage_source =
			"#version 330 core\n"
			"uniform sampler2D u_texture;\n"
			"in vec2 v_texcoords;\n"
			"out vec4 f_color;\n"
			"void main() {\n"
				"f_color = texture(u_texture, v_texcoords);\n"
			"}";
		shader_source.fragment_stage_source_len = strlen(shader_source.p_fragment_stage_source);

		p_shader = CX_CALLOC(sizeof(struct cx_shader));
		cx_shader_set_source(p_shader, &shader_source);

		cx_ed_asset_library_new(CX_ASSET_TYPE_SHADER, "shader_screen_quad", p_shader, &asset_ref);
		cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);
	}

	{
		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/lit.vert",
				(void**)&p_vertex_stage_file_contents,
				&shader_source.vertex_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_vertex_stage_source = p_vertex_stage_file_contents;

		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/lit.frag",
				(void**)&p_fragment_stage_file_contents,
				&shader_source.fragment_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_fragment_stage_source = p_fragment_stage_file_contents;

		p_shader = CX_CALLOC(sizeof(struct cx_shader));
		cx_shader_set_source(p_shader, &shader_source);

		cx_ed_asset_library_new(CX_ASSET_TYPE_SHADER, "shader_lit", p_shader, &asset_ref);
		cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

		cx_io_file_free(p_vertex_stage_file_contents);
		cx_io_file_free(p_fragment_stage_file_contents);
	}

	{
		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/fullscreen_tri.vert",
				(void**)&p_vertex_stage_file_contents,
				&shader_source.vertex_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_vertex_stage_source = p_vertex_stage_file_contents;

		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/ed_grid.frag",
				(void**)&p_fragment_stage_file_contents,
				&shader_source.fragment_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_fragment_stage_source = p_fragment_stage_file_contents;

		p_shader = CX_CALLOC(sizeof(struct cx_shader));
		cx_shader_set_source(p_shader, &shader_source);

		cx_ed_asset_library_new(CX_ASSET_TYPE_SHADER, "shader_editor_grid", p_shader, &asset_ref);
		cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

		cx_io_file_free(p_vertex_stage_file_contents);
		cx_io_file_free(p_fragment_stage_file_contents);
	}

	{
		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/flat.vert",
				(void**)&p_vertex_stage_file_contents,
				&shader_source.vertex_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_vertex_stage_source = p_vertex_stage_file_contents;

		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/flat.frag",
				(void**)&p_fragment_stage_file_contents,
				&shader_source.fragment_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_fragment_stage_source = p_fragment_stage_file_contents;

		p_shader = CX_CALLOC(sizeof(struct cx_shader));
		cx_shader_set_source(p_shader, &shader_source);

		cx_ed_asset_library_new(CX_ASSET_TYPE_SHADER, "shader_flat", p_shader, &asset_ref);
		cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

		cx_io_file_free(p_vertex_stage_file_contents);
		cx_io_file_free(p_fragment_stage_file_contents);
	}

	{
		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/text.vert",
				(void**)&p_vertex_stage_file_contents,
				&shader_source.vertex_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_vertex_stage_source = p_vertex_stage_file_contents;

		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/text.frag",
				(void**)&p_fragment_stage_file_contents,
				&shader_source.fragment_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_fragment_stage_source = p_fragment_stage_file_contents;

		p_shader = CX_CALLOC(sizeof(struct cx_shader));
		cx_shader_set_source(p_shader, &shader_source);

		cx_ed_asset_library_new(CX_ASSET_TYPE_SHADER, "shader_text", p_shader, &asset_ref);
		cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

		cx_io_file_free(p_vertex_stage_file_contents);
		cx_io_file_free(p_fragment_stage_file_contents);
	}

	{
		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/object_id.vert",
				(void**)&p_vertex_stage_file_contents,
				&shader_source.vertex_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_vertex_stage_source = p_vertex_stage_file_contents;

		CX_ASSERT(
			cx_io_file_read_all(
				"res/builtin/shd/object_id.frag",
				(void**)&p_fragment_stage_file_contents,
				&shader_source.fragment_stage_source_len
			) == CX_SUCCESS,
			ED_WORLD_EDITOR);
		shader_source.p_fragment_stage_source = p_fragment_stage_file_contents;

		p_shader = CX_CALLOC(sizeof(struct cx_shader));
		cx_shader_set_source(p_shader, &shader_source);

		cx_ed_asset_library_new(CX_ASSET_TYPE_SHADER, "shader_object_id", p_shader, &asset_ref);
		cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);

		cx_io_file_free(p_vertex_stage_file_contents);
		cx_io_file_free(p_fragment_stage_file_contents);
	}

	struct cx_material* p_material;

	{
		p_material = CX_CALLOC(sizeof(struct cx_material));

		struct cx_asset_ref material_shader_asset_ref;
		cx_ed_asset_library_find_asset_by_name(CX_ASSET_TYPE_SHADER, "shader_lit", &material_shader_asset_ref);

		struct cx_render_pipeline_state render_pipeline_state = {
			.flags =
				CX_RENDER_PIPELINE_FLAG_depth_test_enabled |
				CX_RENDER_PIPELINE_FLAG_depth_writes_enabled,
			.depth_test_func = CX_DEPTH_TEST_FUNC_less,
			.cull_mode = CX_CULL_MODE_back
		};

		struct cx_material_texture_info texture_info = {
			.s_name = "u_texture_albedo"
		};

		struct cx_material_block_member_info block_member_info = {
			.s_name = "u_color",
			.type = CX_GFX_SHADER_PROGRAM_VALUE_TYPE_ivec3
		};

		struct cx_material_block_info block_info = {
			.s_name = "blk_material_properties",
			.p_members = &block_member_info,
			.num_members = 1
		};

		struct cx_material_property_info material_property_info = {
			.p_textures = &texture_info,
			.num_textures = 1,
			.p_blocks = &block_info,
			.num_blocks = 1
		};

		cx_material_create(
			&material_shader_asset_ref,
			&render_pipeline_state,
			&material_property_info,
			p_material);

		cx_material_set_block_member(
			p_material, "blk_material_properties", "u_color", (const float[]) { 1.0f, 1.0f, 1.0f, 1.0f });

		cx_ed_asset_library_new(CX_ASSET_TYPE_MATERIAL, "material_lit", p_material, &asset_ref);
		cx_ed_asset_package_builder_add_asset(&package_builder, &asset_ref);
	}

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

int cx_ed_rebuild_core_asset_package_command(
	const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_args;
	(void)p_context;
	cx_ed_rebuild_core_asset_package();
	return 0;
}

int main(int argc, const char** argv) {
	cx_app_init("cx editor", 960, 640, cx_ed_app_init, argc, argv);
	cx_app_run(cx_ed_app_update, cx_ed_app_draw);
	cx_app_shutdown(cx_ed_app_shutdown);
}
