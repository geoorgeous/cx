#include <stdint.h>
#include <stdlib.h>

#include "asset.h"
#include "cx_bdf.h"
#include "cx_console.h"
#include "cx_console_view.h"
#include "cx_font.h"
#include "cx_gfx_context.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"
#include "cx_gfx_texture.h"
#include "cx_image.h"
#include "cx_io.h"
#include "cx_logging.h"
#include "cx_pixel_format.h"
#include "cx_platform_time.h"
#include "cx_text_mesher.h"
#include "cx_texture_atlas_layout.h"
#include "dev.h"
#include "cx_error.h"
#include "gl.h"
#include "gltf.h"
#include "input.h"
#include "keys.h"
#include "import_gltf.h"
#include "material.h"
#include "matrix.h"
#include "mouse_buttons.h"
#include "physics.h"
#include "platform_window.h"
#include "scene.h"
#include "static_mesh.h"
#include "cx_texture.h"
#include "transform.h"
#include "vector.h"

// Need to conceptualise clear boundaries between:
// - Dev tools
// - Debug tools
// - Editor tools
// - Tools available with non-dev builds
//
// Dev tools: kind of a mixture beteen debug and editor? I think really this should just be a command line interface,
// with some auxillary features like the ability to target things in the world, i.e click them etc.
//
// Debug tools: tools that help debug the engine/game. Things like logging, visualisation stuff, etc
//
// Editor tools: tools that help edit; save and load; and package the game, assets, scenes etc.
//
// DEV CLI COMMANDS

static void platform_window_on_created(struct platform_window*, void*);
static void platform_window_on_key(struct platform_window*, void*, enum key, int, unsigned int);
static void platform_window_on_mouse_button(struct platform_window*, void*, enum mouse_button, int, unsigned int);
static void platform_window_on_mouse_move(struct platform_window*, void*, int, int, unsigned int);
static void platform_window_on_char(struct platform_window*, void*, unsigned int);

static void on_key(const void* p_e, void* p_user_ptr);

void platform_window_on_created(struct platform_window* p_window, void* p_user_ptr) {
	(void)p_user_ptr;

    platform_window_set_on_key_callback(p_window, platform_window_on_key, 0);
    platform_window_set_on_mouse_button_callback(p_window, platform_window_on_mouse_button, 0);
    platform_window_set_on_mouse_move_callback(p_window, platform_window_on_mouse_move, 0);
	platform_window_set_on_char_callback(p_window, platform_window_on_char, 0);
}

void platform_window_on_key(struct platform_window* p_window, void* p_user_ptr, enum key key, int b_is_down, unsigned int mods) {
	(void)p_window;
	(void)p_user_ptr;

    struct input_event_data_key event_data = {
        .key = key,
        .b_is_down = b_is_down,
		.mods = mods
	};
    input_event_broadcast(INPUT_EVENT_key, &event_data);
}

void platform_window_on_mouse_button(struct platform_window* p_window, void* p_user_ptr, enum mouse_button button, int b_is_down, unsigned int mods) {
	(void)p_user_ptr;

    struct input_event_data_mouse_button event_data = {
        .button = button,
        .b_is_down = b_is_down,
		.mods = mods
    };
    platform_window_get_mouse_client_coords(p_window, &event_data.client_pos[0], &event_data.client_pos[1]);
    input_event_broadcast(INPUT_EVENT_mouse_button, &event_data);
}

void platform_window_on_mouse_move(struct platform_window* p_window, void* p_user_ptr, int delta_x, int delta_y, unsigned int mods) {
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
			cx_console_set_is_input_enabled(p_console, !p_console->b_is_input_enabled);
			break;
		}

		case KEY_f10: {
			(dev_mode_is_enabled() ? dev_mode_disable : dev_mode_enable)();
			break;			  
		}

		default: break;
	}
}

int main(int argc, const char* argv[]) {
	(void)argc;
	(void)argv;

    // printf("It's the 9th of September 2025 and I'm writing yet another game engine project.\n");

	//cx_log_cat_set("platform", CX_LOG_LEVEL_TRACE);
	//cx_log_cat_set(CX_LOG_CAT_PLATFORM_WINDOW, CX_LOG_LEVEL_TRACE);
	//cx_log_cat_set(CX_LOG_CAT_HASHTABLE, CX_LOG_LEVEL_TRACE);
	//cx_log_cat_set(CX_LOG_CAT_ALL,         CX_LOG_LEVEL_WARNING);
	cx_log_cat_set(CX_LOG_CAT_ASSET,       CX_LOG_LEVEL_WARNING);
	cx_log_cat_set(CX_LOG_CAT_GFX_PROGRAM, CX_LOG_LEVEL_WARNING);
	cx_log_cat_set(CX_LOG_CAT_GFX_TEXTURE, CX_LOG_LEVEL_WARNING);
	//cx_log_cat_set(CX_LOG_CAT_GLTF,        CX_LOG_LEVEL_WARNING);
	//cx_log_cat_set(CX_LOG_CAT_SCENE,       CX_LOG_LEVEL_WARNING);
	
	cx_console_init(cx_console_get());

	enum cx_error err;

    unsigned int window_size[] = { 1200, 900 };

    struct platform_window platform_window;
    err = platform_window_create(window_size[0], window_size[1], "cx test demo", platform_window_on_created, 0, &platform_window);

	if (err != CX_ERROR_none) {
		return err;
	}

    struct cx_gfx_context gl_context;
    err = cx_gfx_context_create(&platform_window, &gl_context);

	cx_gfx_context_set_swap_interval(&gl_context, 0);

	if (err != CX_ERROR_none) {
		return err;
	}

    // create framebuffer

	uint32_t fb_width = window_size[0] * 0.5f;
	uint32_t fb_height = window_size[1] * 0.5f;

	struct cx_gfx_texture texture_fb_color;
	struct cx_gfx_texture texture_fb_depth_stencil;

	cx_gfx_texture_create(&texture_fb_color, fb_width, fb_height, CX_PIXEL_FORMAT_rgb);
	cx_gfx_texture_create(&texture_fb_depth_stencil, fb_width, fb_height, CX_PIXEL_FORMAT_depth_stencil); 

	struct cx_gfx_framebuffer framebuffer;
	cx_gfx_framebuffer_create(&framebuffer);
	cx_gfx_framebuffer_set_attachment(&framebuffer, CX_GFX_FRAMEBUFFER_ATTACHMENT_color0, &texture_fb_color);
	cx_gfx_framebuffer_set_attachment(&framebuffer, CX_GFX_FRAMEBUFFER_ATTACHMENT_depth_stencil, &texture_fb_depth_stencil);

    // create shader program
	
	struct cx_gfx_program_source program_source = {
		.s_vertex_stage_source = "#version 330 core\n"
			"layout(std140) uniform blk_camera {"
				"mat4 u_projection_matrix;"
				"mat4 u_view_matrix;"
			"};"
			"layout(std140) uniform blk_object {"
				"mat4 u_vertex_matrix;"
			"};"
			"layout (location=0) in vec3 a_pos;"
			"layout (location=1) in vec3 a_normal;"
			"layout (location=3) in vec2 a_texcoords;"
			"out vec3 v_normal;"
			"out vec2 v_texcoords;"
			"void main() {"
				"v_normal = normalize(mat3(transpose(inverse(u_vertex_matrix))) * a_normal);"
				"v_texcoords = a_texcoords;"
				"gl_Position = u_projection_matrix * u_view_matrix * u_vertex_matrix * vec4(a_pos, 1.0);"
			"}",
		.s_fragment_stage_source = "#version 330 core\n"
			"layout(std140) uniform blk_material_properties {"
				"vec3 u_color;"
			"};"
			"uniform sampler2D u_texture_albedo;"
			"in vec3 v_normal;"
			"in vec2 v_texcoords;"
			"out vec4 f_color;"
			"void main() {"
				"const vec3 light_dir = normalize(-vec3(1, 1, 1));"
				"const float ka = 0.3;"
				
				"vec3 texture_rgb = texture(u_texture_albedo, v_texcoords).rgb;"
				"vec3 albedo = texture_rgb * u_color;"

				"float kd = max(dot(v_normal, -light_dir), 0);"

				"f_color = vec4(min((ka + kd), 1) * albedo, 1);"
			"}"
	};

	struct cx_gfx_program program;

	cx_gfx_program_create(&program);
	cx_gfx_program_build(&program, &program_source);

	struct cx_gfx_program_param_block program_pblock_camera;
	struct cx_gfx_program_param_block program_pblock_object;
	struct cx_gfx_program_param_block program_pblock_material;
	struct cx_gfx_program_opaque_param program_opaque_texture_albedo;

	cx_gfx_program_refl_param_block(&program, "blk_camera", &program_pblock_camera);
	cx_gfx_program_refl_param_block(&program, "blk_object", &program_pblock_object);
	cx_gfx_program_refl_param_block(&program, "blk_material_properties", &program_pblock_material);
	cx_gfx_program_refl_opaque_param(&program, "u_texture_albedo", &program_opaque_texture_albedo);

	struct cx_gfx_program_param_buffer program_pbuffer_camera;
	struct cx_gfx_program_param_buffer program_pbuffer_object;
	struct cx_gfx_program_param_buffer program_pbuffer_material;

	cx_gfx_program_param_buffer_create(&program_pbuffer_camera, program_pblock_camera.size_);
	cx_gfx_program_param_buffer_create(&program_pbuffer_object, program_pblock_object.size_);
	cx_gfx_program_param_buffer_create(&program_pbuffer_material, program_pblock_material.size_);

	// create text shader program
	
	struct cx_gfx_program_source program_text_source = {
		.s_vertex_stage_source = "#version 330 core\n"
			"layout(std140) uniform blk_camera {"
				"mat4 u_projection_matrix;"
				"mat4 u_view_matrix;"
			"};"
			"layout(std140) uniform blk_object {"
				"mat4 u_vertex_matrix;"
			"};"
			"layout (location=0) in vec3 a_pos;"
			"layout (location=1) in vec2 a_texcoords;"
			"layout (location=2) in vec4 a_color;"
			"out vec2 v_texcoords;"
			"out vec4 v_color;"
			"void main() {"
				"v_texcoords = a_texcoords;"
				"v_color = a_color;"
				"gl_Position = u_projection_matrix * u_view_matrix * u_vertex_matrix * vec4(a_pos, 1.0);"
			"}",
		.s_fragment_stage_source = "#version 330 core\n"
			"uniform sampler2D u_texture_albedo;"
			"in vec2 v_texcoords;"
			"in vec4 v_color;"
			"out vec4 f_color;"
			"void main() {"
				"float r = texture(u_texture_albedo, v_texcoords).r;"
				"f_color = vec4(v_color.r, v_color.g, v_color.b, v_color.a * r);"
			"}"
	};

	struct cx_gfx_program program_text;

	cx_gfx_program_create(&program_text);
	cx_gfx_program_build(&program_text, &program_text_source);

	struct cx_gfx_program_param_block program_text_pblock_camera;
	struct cx_gfx_program_param_block program_text_pblock_object;
	struct cx_gfx_program_opaque_param program_text_opaque_texture_albedo;

	cx_gfx_program_refl_param_block(&program_text, "blk_camera", &program_text_pblock_camera);
	cx_gfx_program_refl_param_block(&program_text, "blk_object", &program_text_pblock_object);
	cx_gfx_program_refl_opaque_param(&program_text, "u_texture_albedo", &program_text_opaque_texture_albedo);

	struct cx_gfx_program_param_buffer program_text_pbuffer_camera;
	struct cx_gfx_program_param_buffer program_text_pbuffer_object;

	cx_gfx_program_param_buffer_create(&program_text_pbuffer_camera, program_text_pblock_camera.size_);
	cx_gfx_program_param_buffer_create(&program_text_pbuffer_object, program_text_pblock_object.size_);

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

	struct cx_gfx_program program_screen;

	cx_gfx_program_create(&program_screen);
	cx_gfx_program_build(&program_screen, &program_screen_source);

	struct cx_gfx_program_opaque_param program_screen_texture_param;

	cx_gfx_program_refl_opaque_param(&program_screen, "u_texture", &program_screen_texture_param);

    register_asset_type(ASSET_TYPE_IMAGE, "image", sizeof(struct cx_image), 0, 0, 0);
    register_asset_type(ASSET_TYPE_TEXTURE, "texture", sizeof(struct cx_texture), 0, 0, 0);
    register_asset_type(ASSET_TYPE_MATERIAL, "material", sizeof(struct material), 0, 0, 0);
    register_asset_type(ASSET_TYPE_STATIC_MESH, "static_mesh", sizeof(struct static_mesh), 0, 0, (void*)static_mesh_free);
    register_asset_type(ASSET_TYPE_SCENE, "scene", sizeof(struct scene), 0, 0, (void*)scene_destroy);

    struct asset_package asset_package;
    asset_package_init(&asset_package);

	void* p_bdf_buf;
	size_t bdf_buf_size;
	cx_io_file_read_all("res/builtin/font_dbg_8x14.bdf", &p_bdf_buf, &bdf_buf_size);

	struct cx_bdf bdf;
	cx_bdf_parse(p_bdf_buf, &bdf);
	cx_io_file_free(p_bdf_buf);

	struct cx_font font;
	cx_font_build_from_bdf(&bdf, &font);
	cx_bdf_free(&bdf);

	struct cx_image font_atlas_image;
	struct cx_texture_atlas_layout font_atlas_layout;
	font_atlas_layout.p_entries = malloc(sizeof(*font_atlas_layout.p_entries) * CX_FONT_NUM_GLYPHS);
	cx_font_create_atlas(&font, &font_atlas_image, &font_atlas_layout);
	cx_font_free_glyph_bitmap_buffer(&font);

	struct cx_gfx_texture font_atlas_texture;
	cx_gfx_texture_create(&font_atlas_texture, font_atlas_image.width, font_atlas_image.height, CX_PIXEL_FORMAT_red);
	cx_gfx_texture_set_data(&font_atlas_texture, font_atlas_image.p_pixel_data, &font_atlas_image.pixel_data_format);
	free(font_atlas_image.p_pixel_data);

    struct gltf gltf;
    gltf_load_from_file("res/Industrial_exterior_v2.glb", &gltf);

    struct import_gltf_result import_gltf_result;
    import_gltf(&gltf, &asset_package, &import_gltf_result);
    
    struct scene* p_scene = import_gltf_result.p_scenes[0]->asset_.p_data_;

    gltf_free(&gltf);
    import_gltf_free(&import_gltf_result);

    uint8_t white_pixel[] = { 0xFF, 0xFF, 0xFF };
    struct cx_image white_image = {
		.width = 1,
		.height = 1,
		.pixel_data_format = {
			.pixel_format = CX_PIXEL_FORMAT_rgb,
			.pixel_type = CX_PIXEL_TYPE_u8
		},
        .p_pixel_data = white_pixel
    };

	struct cx_gfx_texture texture_white_1x1;
	cx_gfx_texture_create(&texture_white_1x1, white_image.width, white_image.height, CX_PIXEL_FORMAT_rgb);
	cx_gfx_texture_set_data(&texture_white_1x1, white_image.p_pixel_data, &white_image.pixel_data_format);

    float camera_position[3] = { 10, 4, 10 };
    float camera_pitch = 0.3491f;
    float camera_yaw = -0.7854f;

	struct {
		float projection_matrix[16];
		float view_matrix[16];
	} camera;

    matrix_make_identity(camera.projection_matrix);
    matrix_make_identity(camera.view_matrix);

    input_init();

	input_event_subscribe(INPUT_EVENT_key, on_key, 0);

    struct physics_world physics_world;
    physics_world_init(&physics_world);
    physics_world_add_solver(&physics_world, physics_collision_solver_impulse);
    physics_world_add_solver(&physics_world, physics_collision_solver_smooth_positions);

    {
        struct scene_entity* p_new_entity;

        // Sphere
        
        p_new_entity = scene_new_entity(p_scene);

        p_new_entity->transform.position[1] = 3;

        p_new_entity->p_physics_object = physics_world_new_object(&physics_world, &p_new_entity->transform, 1);
        physics_world_new_object_collider(&physics_world, p_new_entity->p_physics_object, PHYSICS_COLLIDER_TYPE_sphere);
        
        // Capsule
        
        p_new_entity = scene_new_entity(p_scene);

        p_new_entity->transform.position[1] = 3;
        p_new_entity->transform.position[0] = 2;

        p_new_entity->p_physics_object = physics_world_new_object(&physics_world, &p_new_entity->transform, 1);
        physics_world_new_object_collider(&physics_world, p_new_entity->p_physics_object, PHYSICS_COLLIDER_TYPE_capsule);
        
        // Hull
        
        p_new_entity = scene_new_entity(p_scene);

        p_new_entity->transform.position[1] = 3;
        p_new_entity->transform.position[0] = -2;

        p_new_entity->p_physics_object = physics_world_new_object(&physics_world, &p_new_entity->transform, 1);
        physics_world_new_object_collider(&physics_world, p_new_entity->p_physics_object, PHYSICS_COLLIDER_TYPE_hull);
        
        // Plane

        p_new_entity = scene_new_entity(p_scene);

        p_new_entity->p_physics_object = physics_world_new_object(&physics_world, &p_new_entity->transform, 0);
        physics_world_new_object_collider(&physics_world, p_new_entity->p_physics_object, PHYSICS_COLLIDER_TYPE_plane);
    }
    
    dev_init(&platform_window, p_scene, &physics_world);
	dev_mode_enable();

    uint64_t old_frame_start = cx_platform_time_now();

    while (platform_window_is_open(&platform_window)) {
        const uint64_t frame_start = cx_platform_time_now();
        const double frame_delta_seconds = cx_platform_time_delta_seconds(old_frame_start, frame_start);

		//CX_DBG(CX_LOG_FMT(INFO, DONTCARE, "FRAME TIME = %fms\n",
		//	cx_platform_time_delta_milliseconds(old_frame_start, frame_start)));
        
		old_frame_start = frame_start;

        input_frame_reset();
        platform_window_poll_events(&platform_window);

        // STATE UPDATE LOGIC
        {
            matrix_make_perspective_projection(
				1,
				(float)fb_width / fb_height,
				0.01f, 1000.0f,
				camera.projection_matrix);

            float move_direction[3] = {0};
            if (input_frame_is_key_down(KEY_a)) {
                move_direction[0] -= 1;
            }
            if (input_frame_is_key_down(KEY_d)) {
                move_direction[0] += 1;
            }
            if (input_frame_is_key_down(KEY_s)) {
                move_direction[2] += 1;
            }
            if (input_frame_is_key_down(KEY_w)) {
                move_direction[2] -= 1;
            }
            if (input_frame_is_key_down(KEY_space)) {
                move_direction[1] += 1;
            }
            if (input_frame_is_key_down(KEY_ctrl_left)) {
                move_direction[1] -= 1;
            }

            if (input_frame_is_mouse_button_down(MOUSE_BUTTON_right)) {
                int mouse_delta_x;
                int mouse_delta_y;
                input_frame_mouse_delta(&mouse_delta_x, &mouse_delta_y);

                camera_pitch += mouse_delta_y * 0.01f;
                camera_yaw += mouse_delta_x * 0.01f;
            }
            
            float pitch_rotation_matrix[16];
            matrix_make_rotation_x(camera_pitch, pitch_rotation_matrix);

            float yaw_rotation_matrix[16];
            matrix_make_rotation_y(camera_yaw, yaw_rotation_matrix);
            
            float rotation_matrix[16];
            matrix_multiply(pitch_rotation_matrix, yaw_rotation_matrix, rotation_matrix);

            if (!vec3_is_zero(move_direction)) {
                const float speed = input_frame_is_key_down(KEY_shift_left) ? 50.f : 7.f;
                float offset[4] = { 0, 0, 0, 1 };

                vec3_norm(move_direction, move_direction);
                vec3_mul_s(move_direction, speed * frame_delta_seconds, offset);

                float camera_x_axis[3];
                camera_x_axis[0] = rotation_matrix[0];
                camera_x_axis[1] = rotation_matrix[4];
                camera_x_axis[2] = rotation_matrix[8];

                float camera_y_axis[3];
                camera_y_axis[0] = rotation_matrix[1];
                camera_y_axis[1] = rotation_matrix[5];
                camera_y_axis[2] = rotation_matrix[9];

                float camera_z_axis[3];
                camera_z_axis[0] = rotation_matrix[2];
                camera_z_axis[1] = rotation_matrix[6];
                camera_z_axis[2] = rotation_matrix[10];

                float offset_x[3];
                vec3_mul_s(camera_x_axis, offset[0], offset_x);
                
                float offset_y[3];
                vec3_mul_s(camera_y_axis, offset[1], offset_y);
                
                float offset_z[3];
                vec3_mul_s(camera_z_axis, offset[2], offset_z);

                vec3_add(camera_position, offset_x, camera_position);
                vec3_add(camera_position, offset_y, camera_position);
                vec3_add(camera_position, offset_z, camera_position);
            }
            
            float translation_matrix[16];
            matrix_make_translation(-camera_position[0], -camera_position[1], -camera_position[2], translation_matrix);

            matrix_multiply(rotation_matrix, translation_matrix, camera.view_matrix);

            if (input_frame_is_key_down(KEY_p)) {
                physics_world_step(&physics_world, frame_delta_seconds);
            }
        }

        // DRAW
        {
            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST); 
            glViewport(0, 0, fb_width, fb_height);
			cx_gfx_framebuffer_bind(&framebuffer);
            glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
			cx_gfx_program_bind(&program);
			
			cx_gfx_program_param_block_bind_buffer(&program_pblock_camera, &program_pbuffer_camera, 0, 0);
			cx_gfx_program_param_block_bind_buffer(&program_pblock_object, &program_pbuffer_object, 0, 0);
			cx_gfx_program_param_block_bind_buffer(&program_pblock_material, &program_pbuffer_material, 0, 0);

			cx_gfx_program_param_buffer_set(&program_pbuffer_camera, 0, 0, &camera);
            
            for (size_t i = 0; i < p_scene->entities_.length_; ++i) {
                struct scene_entity* p_entity = *(struct scene_entity**)darr_get(&p_scene->entities_, i);

                transform_compute_world_trs_matrix(&p_entity->transform);

                if (!p_entity->p_mesh) {
                    continue;
                }

				cx_gfx_program_param_buffer_set(&program_pbuffer_object, 0, 0, p_entity->transform.world_trs_matrix);

                float model_color[] = { 1, 1, 1 };
				cx_gfx_program_param_buffer_set(&program_pbuffer_material, 0, 0, model_color);

                struct static_mesh* p_mesh = p_entity->p_mesh->asset_.p_data_;

                if (!p_mesh->b_loaded_device_meshes) {
                    static_mesh_load_device_meshes(p_mesh);
                }

                for (size_t j = 0; j < p_mesh->num_primitives; ++j) {
					const struct cx_gfx_texture* p_gfx_texture = &texture_white_1x1;

                    if (p_mesh->p_materials[j]) {
                        const struct material* p_material = p_mesh->p_materials[j]->asset_.p_data_;
                        if (p_material->p_texture) {
                            struct cx_texture* p_texture = p_material->p_texture->asset_.p_data_;
                            cx_texture_load_gfx_texture(p_texture, 0);
							p_gfx_texture = &p_texture->gfx_texture_;
                        }
                    }
					
					cx_gfx_program_opaque_param_bind_resource(&program_opaque_texture_albedo, p_gfx_texture);
                    cx_gfx_mesh_draw(&p_mesh->p_gfx_meshes[j]);
                }
            }

        	dev_draw(&framebuffer, fb_width, fb_height, camera.projection_matrix, camera.view_matrix);

			if (cx_console_get()->b_is_input_enabled) {
				cx_gfx_framebuffer_bind(&framebuffer);

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glClear(GL_DEPTH_BUFFER_BIT);

				struct {
					float projection_matrix[16];
					float view_matrix[16];
				} ui_camera;

				matrix_make_orthographic_projection(0, fb_width, fb_height, 0, -1, 1, ui_camera.projection_matrix);
				matrix_make_identity(ui_camera.view_matrix);

				struct cx_font_render_data font_render_data = {
					.p_font = &font,
					.p_glyph_texture = &font_atlas_texture,
					.p_glyph_atlas_layout = &font_atlas_layout
				};
				cx_console_view_draw(cx_console_get(), &font_render_data, ui_camera.projection_matrix, ui_camera.view_matrix);
			}

            // SCREEN QUAD
            {
                platform_window_size(&platform_window, &window_size[0], &window_size[1]);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, (GLsizei)window_size[0], (GLsizei)window_size[1]);
				glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				cx_gfx_program_bind(&program_screen);
		
				cx_gfx_program_opaque_param_bind_resource(&program_screen_texture_param, &texture_fb_color);

                GLuint gl_empty_vao;
                glGenVertexArrays(1, &gl_empty_vao);
                glBindVertexArray(gl_empty_vao);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glDeleteVertexArrays(1, &gl_empty_vao);
            }
        }

		cx_gfx_context_swap_buffers(&gl_context);
    }

    return 0;
}
