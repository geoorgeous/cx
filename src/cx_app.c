#include <stdint.h>
#include <time.h>

#include "cx_app.h"
#include "cx_asset_cache.h"
#include "cx_asset_defs.h"
#include "cx_asset_package.h"
#include "cx_blueprint.h"
#include "cx_cmp_collider.h"
#include "cx_cmp_rigidbody.h"
#include "cx_cmp_static_mesh.h"
#include "cx_command.h"
#include "cx_component.h"
#include "cx_console.h"
#include "cx_console_view.h"
#include "cx_font.h"
#include "cx_gfx_context.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_render_pass.h"
#include "cx_gfx_texture.h"
#include "cx_image.h"
#include "cx_input.h"
#include "cx_keys.h"
#include "cx_logging.h"
#include "cx_material.h"
#include "cx_pixel_format.h"
#include "cx_platform_time.h"
#include "cx_shader.h"
#include "cx_text_mesher.h"
#include "cx_texture.h"
#include "cx_texture_atlas_layout.h"
#include "cx_world.h"
#include "cx_world_blueprint.h"
#include "input.h"
#include "keys.h"
#include "matrix.h"
#include "static_mesh.h"

static struct {
	struct cx_platform_window window;
	struct cx_gfx_context gfx_context;

	struct cx_gfx_framebuffer primary_framebuffer;
	struct cx_gfx_texture primary_framebuffer_texture_color;
	struct cx_gfx_texture primary_framebuffer_texture_depth_stencil;

	struct cx_asset_ref asset_ref_shader_screen_quad;
	struct cx_gfx_mesh dummy_mesh;

	struct cx_asset_package builtin_asset_pkg;
	struct cx_asset_ref console_font_ref;
	struct cx_texture_atlas_layout console_font_glyph_atlas_layout;
	struct cx_texture_atlas_entry console_font_glyph_atlas_layout_entries[CX_FONT_NUM_GLYPHS];
	struct cx_gfx_texture console_font_glyph_atlas_texture;
} cx_app;

static int console_command_quit(const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static int cx_asset_source_get_package_asset_name(cx_asset_id id, void* p_context, const char** pp_out);
static int cx_asset_source_find_package_asset_by_name(
	cx_asset_type type, const char* s_name, void* p_context, struct cx_asset_ref* p_out_ref);
static int cx_asset_source_deserialize_package_asset(cx_asset_id id, void* p_context, void* p_out);

int cx_app_init(
	const char* s_name,
	uint32_t window_width,
	uint32_t window_height,
	cx_app_init_callback_fn f_init,
	int argc,
	const char** argv) {

	cx_result result;

	srand((unsigned int)time(CX_NULL));

	result = cx_platform_window_create(
		window_width, window_height,
		s_name,
		&cx_app.window);

	if (result != CX_SUCCESS) {
		return result;
	}

	result = cx_gfx_context_create(&cx_app.window, &cx_app.gfx_context);

	cx_gfx_context_set_swap_interval(&cx_app.gfx_context, 0);

	if (result != CX_SUCCESS) {
		return result;
	}

	// create framebuffer

	const float resolution_scale = 1.0f;
	uint32_t fb_width = (uint32_t)((float)window_width * resolution_scale);
	uint32_t fb_height = (uint32_t)((float)window_height * resolution_scale);

	(void)cx_gfx_framebuffer_create(&cx_app.primary_framebuffer);

	cx_gfx_texture_create(
		&cx_app.primary_framebuffer_texture_color, fb_width, fb_height, CX_PIXEL_FORMAT_rgb);
	cx_gfx_framebuffer_set_attachment(
		&cx_app.primary_framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
		&cx_app.primary_framebuffer_texture_color);

	cx_gfx_texture_create(
		&cx_app.primary_framebuffer_texture_depth_stencil, fb_width, fb_height, CX_PIXEL_FORMAT_depth_stencil); 
	cx_gfx_framebuffer_set_attachment(
		&cx_app.primary_framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_depth_stencil,
		&cx_app.primary_framebuffer_texture_depth_stencil);

	// create screen shader program
	

	// TODO(george): for materials that are saved (like entity mesh materials) we need to make sure that the shaders
	//     they use are also saved assets. for other built-in shaders, they don't necessarily need to be saved, like
	//     screen-quad, mesh-picker, or UI shaders
	// TODO(george): refactor screen quad code to use new material/shader
	// TODO(george): refactor mesh picker code to use new material/shader
	// TODO(george): refactor world renderer to use new material and render param code

	cx_asset_register_type(CX_ASSET_TYPE_IMAGE, "image", sizeof(struct cx_image),
		cx_image_asset_serialize, cx_image_asset_deserialize, CX_NULL, cx_image_asset_destroy);

	cx_asset_register_type(CX_ASSET_TYPE_TEXTURE, "texture", sizeof(struct cx_texture),
		cx_texture_asset_serialize,
		cx_texture_asset_deserialize,
		cx_texture_asset_enumerate_dependencies,
		cx_texture_asset_free);

	cx_asset_register_type(CX_ASSET_TYPE_SHADER, "shader", sizeof(struct cx_shader), 
		cx_shader_asset_serialize, cx_shader_asset_deserialize, CX_NULL, cx_shader_asset_free);

	cx_asset_register_type(CX_ASSET_TYPE_MATERIAL, "material", sizeof(struct cx_material),
		cx_material_asset_serialize, 
		cx_material_asset_deserialize,
		cx_material_asset_enumerate_dependencies,
		cx_material_asset_free);
	
	cx_asset_register_type(CX_ASSET_TYPE_STATIC_MESH, "static_mesh", sizeof(struct static_mesh),
		static_mesh_asset_serialize,
		static_mesh_asset_deserialize,
		static_mesh_asset_enumerate_dependencies,
		static_mesh_asset_free);
	
	cx_asset_register_type(CX_ASSET_TYPE_FONT, "font", sizeof(struct cx_font),
		cx_font_asset_serialize, cx_font_asset_deserialize, CX_NULL, cx_font_asset_destroy);
	
	cx_asset_register_type(CX_ASSET_TYPE_BLUEPRINT, "blueprint", sizeof(struct cx_blueprint),
		cx_blueprint_asset_serialize,
		cx_blueprint_asset_deserialize,
		cx_blueprint_asset_enumerate_dependencies,
		cx_blueprint_asset_free);

	cx_asset_register_type(CX_ASSET_TYPE_WORLD_BLUEPRINT, "world_blueprint", sizeof(struct cx_world_blueprint),
		cx_world_blueprint_asset_serialize,
		cx_world_blueprint_asset_deserialize,
		CX_NULL,
		cx_world_blueprint_asset_free);
	
	cx_component_register_type(&cmp_type_static_mesh);
	cx_component_register_type(&cmp_type_collider);
	cx_component_register_type(&cmp_type_rigidbody);

	if (cx_asset_package_import("res/builtin/core.cxpkg", &cx_app.builtin_asset_pkg)) {
		cx_asset_cache_push_source(&(struct cx_asset_source) {
			.p_context = &cx_app.builtin_asset_pkg,
			.f_get_asset_name = cx_asset_source_get_package_asset_name,
			.f_find_asset_by_name = cx_asset_source_find_package_asset_by_name,
			.f_try_deserialize_asset = cx_asset_source_deserialize_package_asset
		});
	}

	input_init();

	input_event_subscribe(INPUT_EVENT_key, on_key, 0);
	
	cx_console_init(cx_console_get());

	CX_NEW_CONSOLE_COMMAND("quit", "Close application", console_command_quit, CX_NULL, CX_CONSOLE_COMMAND_NO_PARAMS);
	CX_NEW_CONSOLE_COMMAND_ALIAS("q", "quit");
	
	f_init(argc, argv);
	
	{
		cx_asset_cache_find_by_name(CX_ASSET_TYPE_FONT, "default_8x14", &cx_app.console_font_ref);
		struct cx_font* p_font = cx_asset_cache_acquire(&cx_app.console_font_ref);

		cx_app.console_font_glyph_atlas_layout.p_entries = cx_app.console_font_glyph_atlas_layout_entries;

		struct cx_image font_atlas_image;
		cx_font_create_atlas(p_font, &font_atlas_image, &cx_app.console_font_glyph_atlas_layout);

		cx_gfx_texture_create(
			&cx_app.console_font_glyph_atlas_texture,
			font_atlas_image.width, font_atlas_image.height,
			CX_PIXEL_FORMAT_red);

		cx_gfx_texture_set_data(
			&cx_app.console_font_glyph_atlas_texture,
			font_atlas_image.p_pixel_data,
			&font_atlas_image.pixel_data_format);

		free(font_atlas_image.p_pixel_data);
	}

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_screen_quad", &cx_app.asset_ref_shader_screen_quad);
	struct cx_shader* p_shader = cx_asset_cache_acquire(&cx_app.asset_ref_shader_screen_quad);
	cx_shader_load_device_program(p_shader);

	const struct cx_mesh_data dummy_mesh_data = {
		.layout.draw_mode = CX_MESH_DRAW_MODE_triangles,
		.vertex_count = 3
	};
	cx_gfx_mesh_create(&dummy_mesh_data, CX_GFX_BUFFER_USAGE_static, &cx_app.dummy_mesh);

	return 0;
}

void cx_app_run(cx_app_update_callback_fn f_update, cx_app_draw_callback_fn f_draw) {
	uint64_t old_frame_start = cx_platform_time_now();

	while (cx_platform_window_is_open(&cx_app.window)) {
		const uint64_t frame_start = cx_platform_time_now();
		const double frame_delta_seconds = cx_platform_time_delta_seconds(old_frame_start, frame_start);

		old_frame_start = frame_start;

		cx_platform_window_process_events(&cx_app.window);

		if (!cx_platform_window_is_open(&cx_app.window)) {
			break;
		}

		cx_input_sample(&cx_app.window);

		if (cx_input_was_key_pressed(CX_KEY_grave)) {
			cx_console_set_is_input_enabled(cx_console_get(), 1);
		}

		cx_console_update(cx_console_get());

		if (cx_platform_window_was_focus_changed(&cx_app.window)) {
			CX_LAZYLOG_FMT("Window foucs changed: %d\n", cx_platform_window_is_focused(&cx_app.window));
		}

		f_update(frame_delta_seconds);

		// DRAW
		{
			f_draw(&cx_app.primary_framebuffer);

			if (cx_console_get()->b_is_input_enabled) {
				struct cx_font_render_data font_render_data = {
					.p_font = cx_asset_cache_acquire(&cx_app.console_font_ref),
					.p_glyph_texture = &cx_app.console_font_glyph_atlas_texture,
					.p_glyph_atlas_layout = &cx_app.console_font_glyph_atlas_layout
				};

				float projection_matrix[16];
				float view_matrix[16];

				matrix_make_orthographic_projection(
					 0,
					(float)cx_app.primary_framebuffer_texture_color.width_,
					(float)cx_app.primary_framebuffer_texture_color.height_,
					 0,
					-1,
					 1,
					projection_matrix);
				matrix_make_identity(view_matrix);

				cx_console_view_draw(
					cx_console_get(),
					&font_render_data,
					&cx_app.primary_framebuffer,
					cx_app.primary_framebuffer_texture_color.width_,
					cx_app.primary_framebuffer_texture_color.height_,
					projection_matrix, view_matrix);
			}

			// SCREEN QUAD
			{
				uint32_t window_width;
				uint32_t window_height;
				platform_window_size(&cx_app.window, &window_width, &window_height);

				struct cx_gfx_shader_program_input_texture shader_input_texture = {
					.s_name = "u_texture",
					.p_texture = &cx_app.primary_framebuffer_texture_color
				};

				struct cx_gfx_render_pass render_pass_screen_quad = {
					.p_framebuffer = cx_gfx_context_get_backbuffer(&cx_app.gfx_context),
					.viewport = { 0, 0, (int32_t)window_width, (int32_t)window_height },
					.clear_flags =
						CX_GFX_RENDER_TARGET_CLEAR_FLAG_color |
						CX_GFX_RENDER_TARGET_CLEAR_FLAG_depth,
					.pass_input_set = {
						.p_textures = &shader_input_texture,
						.num_textures = 1
					}
				};

				struct cx_render_draw_command draw_command_screen_quad = {
					.pipeline = {
						.p_shader = cx_asset_ref_get(&cx_app.asset_ref_shader_screen_quad)
					},
					.p_mesh = &cx_app.dummy_mesh
				};

				cx_gfx_render_pass_execute(&render_pass_screen_quad, &draw_command_screen_quad, 1);
			}
		}

		cx_gfx_context_swap_buffers(&cx_app.gfx_context);
	}
}

void cx_app_shutdown(cx_app_shutdown_callback_fn f_shutdown) {
	f_shutdown();

	cx_asset_cache_free();

	cx_gfx_context_destroy(&cx_app.gfx_context);

	CX_LOG(INFO, DONTCARE, "Exiting\n");
}

struct cx_platform_window* cx_app_primary_window(void) {
	return &cx_app.window;
}

int console_command_quit(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_args;
	(void)p_context;
	cx_platform_window_destroy(&cx_app.window);
	return 0;
}

int cx_asset_source_get_package_asset_name(cx_asset_id id, void* p_context, const char** pp_out) {
	const struct cx_asset_package* p_package = p_context;
	return cx_asset_package_get_asset_name(p_package, id, pp_out);
}

int cx_asset_source_find_package_asset_by_name(
	cx_asset_type type, const char* s_name, void* p_context, struct cx_asset_ref* p_out_ref) {
	
	const struct cx_asset_package* p_package = p_context;
	return cx_asset_package_find_asset_by_name(p_package, type, s_name, p_out_ref);
}

int cx_asset_source_deserialize_package_asset(cx_asset_id id, void* p_context, void* p_out) {
	const struct cx_asset_package* p_package = p_context;
	return cx_asset_package_deserialize_asset(p_package, id, p_out);
}
