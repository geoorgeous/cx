#include <stdint.h>

#include "cx_app.h"
#include "cx_asset_cache.h"
#include "cx_asset_package.h"
#include "cx_asset_types.h"
#include "cx_blueprint.h"
#include "cx_cmp_collider.h"
#include "cx_cmp_rigidbody.h"
#include "cx_cmp_static_mesh.h"
#include "cx_command.h"
#include "cx_component.h"
#include "cx_console.h"
#include "cx_console_view.h"
#include "cx_error.h"
#include "cx_font.h"
#include "cx_gfx_context.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_program.h"
#include "cx_gfx_texture.h"
#include "cx_image.h"
#include "cx_logging.h"
#include "cx_pixel_format.h"
#include "cx_platform_time.h"
#include "cx_text_mesher.h"
#include "cx_texture.h"
#include "cx_texture_atlas_layout.h"
#include "cx_world.h"
#include "cx_world_blueprint.h"
#include "gl.h"
#include "input.h"
#include "keys.h"
#include "material.h"
#include "matrix.h"
#include "mouse_buttons.h"
#include "platform_window.h"
#include "static_mesh.h"

static struct {
	struct platform_window window;
	struct cx_gfx_context  gfx_context;

	struct cx_gfx_framebuffer primary_framebuffer;
	struct cx_gfx_texture primary_framebuffer_texture_color;
	struct cx_gfx_texture primary_framebuffer_texture_depth_stencil;

	struct cx_gfx_program screen_quad_program;
	struct cx_gfx_program_opaque_param screen_quad_program_opaque_param_texture;

	struct cx_asset_package builtin_asset_pkg;
	struct cx_asset_ref console_font_ref;
	struct cx_texture_atlas_layout console_font_glyph_atlas_layout;
	struct cx_texture_atlas_entry console_font_glyph_atlas_layout_entries[CX_FONT_NUM_GLYPHS];
	struct cx_gfx_texture console_font_glyph_atlas_texture;
} cx_app;

static void platform_window_on_created(struct platform_window*, void*);
static void platform_window_on_close(struct platform_window*, void*);
static void platform_window_on_key(struct platform_window*, void*, enum key, int, unsigned int);
static void platform_window_on_mouse_button(struct platform_window*, void*, enum mouse_button, int, unsigned int);
static void platform_window_on_mouse_move(struct platform_window*, void*, int, int, unsigned int);
static void platform_window_on_char(struct platform_window*, void*, unsigned int);

static void on_key(const void* p_e, void* p_user_ptr);

static int console_command_quit(const struct cx_command_args* p_args, const struct cx_command_context* p_context);

static int cx_asset_source_get_package_asset_name(cx_asset_id id, void* p_context, const char** pp_out);
static int cx_asset_source_find_package_asset_by_name(
	cx_asset_type type, const char* s_name, void* p_context, struct cx_asset_ref* p_out_ref);
static int cx_asset_source_deserialize_package_asset(cx_asset_id id, void* p_context, void* p_out);

int cx_app_init(const char* s_name, uint32_t window_width, uint32_t window_height, cx_app_init_callback_fn f_init) {
	enum cx_error err;

	err = platform_window_create(
		window_width, window_height,
		s_name,
		platform_window_on_created,
		0,
		&cx_app.window);

	CX_NEW_CONSOLE_COMMAND("quit", "Close application", console_command_quit, CX_NULL, CX_CONSOLE_COMMAND_NO_PARAMS);
	CX_NEW_CONSOLE_COMMAND_ALIAS("q", "quit");

	if (err != CX_ERROR_none) {
		return (int)err;
	}

	err = cx_gfx_context_create(&cx_app.window, &cx_app.gfx_context);

	cx_gfx_context_set_swap_interval(&cx_app.gfx_context, 0);

	if (err != CX_ERROR_none) {
		return (int)err;
	}

	// create framebuffer

	const float resolution_scale = 1.0f;
	uint32_t fb_width = (uint32_t)((float)window_width * resolution_scale);
	uint32_t fb_height = (uint32_t)((float)window_height * resolution_scale);

	cx_gfx_framebuffer_create(&cx_app.primary_framebuffer);

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
	
	struct cx_gfx_program_source program_screen_source = {
		.s_vertex_stage_source = "#version 330 core\n"
			"out vec2 v_texcoords;\n"
			"void main() {\n"
				"vec2 vertices[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));\n"
				"gl_Position = vec4(vertices[gl_VertexID], 0, 1);\n"
				"v_texcoords = 0.5 * gl_Position.xy + vec2(0.5);\n"
			"}",
		.s_fragment_stage_source = "#version 330 core\n"
		"uniform sampler2D u_texture;\n"
		"in vec2 v_texcoords;\n"
		"out vec4 f_color;\n"
		"void main() {\n"
			"f_color = texture(u_texture, v_texcoords);\n"
		"}"
	};

	cx_gfx_program_create(&cx_app.screen_quad_program);
	cx_gfx_program_build(&cx_app.screen_quad_program, &program_screen_source);

	cx_gfx_program_refl_opaque_param(
		&cx_app.screen_quad_program,
		"u_texture",
		&cx_app.screen_quad_program_opaque_param_texture);

	cx_asset_register_type(CX_ASSET_TYPE_IMAGE, "image", sizeof(struct cx_image),
		cx_image_asset_serialize, cx_image_asset_deserialize, CX_NULL, cx_image_asset_destroy);

	cx_asset_register_type(CX_ASSET_TYPE_TEXTURE, "texture", sizeof(struct cx_texture),
		cx_texture_asset_serialize,
		cx_texture_asset_deserialize,
		cx_texture_asset_enumerate_dependencies,
		cx_texture_asset_free);

	cx_asset_register_type(CX_ASSET_TYPE_MATERIAL, "material", sizeof(struct material),
		material_asset_serialize, material_asset_deserialize, material_asset_enumerate_dependencies, CX_NULL);
	
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
		cx_world_blueprint_asset_serialize, cx_world_blueprint_asset_deserialize, CX_NULL, CX_NULL);
	
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

		cx_asset_cache_find_by_name(CX_ASSET_TYPE_FONT, "default_8x14", &cx_app.console_font_ref);
		struct cx_font* p_font = cx_asset_cache_acquire(&cx_app.console_font_ref);

		cx_app.console_font_glyph_atlas_layout.p_entries = cx_app.console_font_glyph_atlas_layout_entries;

		struct cx_image font_atlas_image;
		cx_font_create_atlas(p_font, &font_atlas_image, &cx_app.console_font_glyph_atlas_layout);

		cx_font_free_glyph_bitmap_buffer(p_font);

		cx_gfx_texture_create(
			&cx_app.console_font_glyph_atlas_texture,
			font_atlas_image.width, font_atlas_image.height,
			CX_PIXEL_FORMAT_red);

		cx_gfx_texture_set_data(
			&cx_app.console_font_glyph_atlas_texture,
			font_atlas_image.p_pixel_data,
			&font_atlas_image.pixel_data_format);

		free(font_atlas_image.p_pixel_data);

		cx_console_init(cx_console_get());
	}

	input_init();

	input_event_subscribe(INPUT_EVENT_key, on_key, 0);

	f_init();

	return 0;
}

void cx_app_run(cx_app_update_callback_fn f_update, cx_app_draw_callback_fn f_draw) {
	uint64_t old_frame_start = cx_platform_time_now();

	while (platform_window_is_open(&cx_app.window)) {
		const uint64_t frame_start = cx_platform_time_now();
		const double frame_delta_seconds = cx_platform_time_delta_seconds(old_frame_start, frame_start);

		old_frame_start = frame_start;

		input_frame_reset();
		platform_window_poll_events(&cx_app.window);

		if (!platform_window_is_open(&cx_app.window)) {
			break;
		}

		f_update(frame_delta_seconds);

		// DRAW
		{
			f_draw(&cx_app.primary_framebuffer);

			if (cx_console_get()->b_is_input_enabled) {
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

				cx_console_view_draw(cx_console_get(),
					&font_render_data,
					&cx_app.primary_framebuffer,
					cx_app.primary_framebuffer_texture_color.width_,
					cx_app.primary_framebuffer_texture_color.height_,
					projection_matrix, view_matrix);
			}

			// SCREEN QUAD
			{
				uint32_t window_size[2];
				platform_window_size(&cx_app.window, &window_size[0], &window_size[1]);

				cx_gfx_framebuffer_bind(cx_gfx_context_get_backbuffer(&cx_app.gfx_context));

				glViewport(0, 0, (GLsizei)window_size[0], (GLsizei)window_size[1]);
				glClearColor(0, 0, 0, 0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				cx_gfx_program_bind(&cx_app.screen_quad_program);
		
				cx_gfx_program_opaque_param_bind_resource(&((struct cx_gfx_program_opaque_param_binding){
					.p_param = &cx_app.screen_quad_program_opaque_param_texture,
					.p_resource = &cx_app.primary_framebuffer_texture_color
				}));

				GLuint gl_empty_vao;
				glGenVertexArrays(1, &gl_empty_vao);
				glBindVertexArray(gl_empty_vao);
				glDrawArrays(GL_TRIANGLES, 0, 3);
				glDeleteVertexArrays(1, &gl_empty_vao);
			}
		}

		cx_gfx_context_swap_buffers(&cx_app.gfx_context);
	}
}

void cx_app_shutdown(cx_app_shutdown_callback_fn f_shutdown) {
	f_shutdown();

	cx_asset_cache_free();

	CX_LOG(INFO, DONTCARE, "Exiting\n");
}

struct platform_window* cx_app_primary_window(void) {
	return &cx_app.window;
}

void platform_window_on_created(struct platform_window* p_window, void* p_user_ptr) {
	(void)p_user_ptr;

	platform_window_set_on_close_callback(p_window, platform_window_on_close, CX_NULL);
	platform_window_set_on_key_callback(p_window, platform_window_on_key, CX_NULL);
	platform_window_set_on_mouse_button_callback(p_window, platform_window_on_mouse_button, CX_NULL);
	platform_window_set_on_mouse_move_callback(p_window, platform_window_on_mouse_move, CX_NULL);
	platform_window_set_on_char_callback(p_window, platform_window_on_char, CX_NULL);
}

void platform_window_on_close(struct platform_window* p_window, void* p_user_ptr) {
	(void)p_window;
	(void)p_user_ptr;

	cx_gfx_context_destroy(&cx_app.gfx_context);
}

void platform_window_on_key(
	struct platform_window* p_window,
	void* p_user_ptr,
	enum key key,
	int b_is_down,
	unsigned int mods) {

	(void)p_window;
	(void)p_user_ptr;

	struct input_event_data_key event_data = {
		.key = key,
		.b_is_down = b_is_down,
		.mods = mods
	};
	input_event_broadcast(INPUT_EVENT_key, &event_data);
}

void platform_window_on_mouse_button(
	struct platform_window* p_window,
	void* p_user_ptr,
	enum mouse_button button,
	int b_is_down,
	unsigned int mods) {

	(void)p_user_ptr;

	struct input_event_data_mouse_button event_data = {
		.button = button,
		.b_is_down = b_is_down,
		.mods = mods
	};
	platform_window_get_mouse_client_coords(p_window, &event_data.client_pos[0], &event_data.client_pos[1]);
	input_event_broadcast(INPUT_EVENT_mouse_button, &event_data);
}

void platform_window_on_mouse_move(
	struct platform_window* p_window,
	void* p_user_ptr,
	int delta_x,
	int delta_y,
	unsigned int mods) {

	(void)p_window;
	(void)p_user_ptr;

	struct input_event_data_mouse_move event_data = {
		.delta_x = delta_x,
		.delta_y = delta_y,
		.mods = mods
	};
	input_event_broadcast(INPUT_EVENT_mouse_move, &event_data);
}

void platform_window_on_char(struct platform_window* p_window, void* p_user_ptr, unsigned int code) {
	(void)p_window;
	(void)p_user_ptr;

	struct input_event_data_char event_data = {
		.code = code
	};
	input_event_broadcast(INPUT_EVENT_char, &event_data);
}

void on_key(const void* p_e, void* p_user_ptr) {
	(void)p_user_ptr;

	const struct input_event_data_key* p_key_event = p_e;

	if (p_key_event->b_is_down) {
		return;
	}

	switch (p_key_event->key) {
		case KEY_grave: {
			struct cx_console* p_console = cx_console_get();
			cx_console_set_is_input_enabled(p_console, 1);
			break;
		}

		default: break;
	}
}

int console_command_quit(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_args;
	(void)p_context;
	platform_window_destroy(&cx_app.window);
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
