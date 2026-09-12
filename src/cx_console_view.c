#include "cx_asset_cache.h"
#include "cx_console.h"
#include "cx_console_view.h"
#include "cx_font.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_render_pass.h"
#include "cx_mesh_data.h"
#include "cx_mesh_gen.h"
#include "cx_render_draw_command.h"
#include "cx_shader.h"
#include "cx_text_mesher.h"
#include "matrix.h"

#define CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS 1024

static struct cx_render_pipeline g_render_pipeline_flat;
static struct cx_render_pipeline g_render_pipeline_text;

static struct cx_gfx_mesh g_text_mesh;
static struct cx_gfx_mesh g_log_text_mesh;
static struct cx_gfx_mesh g_quad_mesh;

static struct cx_render_draw_command g_render_draw_commands[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS];

static struct cx_gfx_shader_program_input_texture
	g_shader_program_input_texture_pool[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS];
static uint16_t g_shader_program_input_texture_pool_len;

static struct cx_gfx_shader_program_input_block
	g_shader_program_input_block_pool[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS];
static uint16_t g_shader_program_input_block_pool_len;

static float g_draw_command_vertex_matrices[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS][16];
static float g_draw_command_colors[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS][4];

static int cx_console_view_init(void);

static void cx_console_view_generate_text_meshes(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_font_render_data);

static void cx_console_view_record_quad(
	struct cx_render_draw_command_buffer* p_render_draw_command_buffer,
	float x, float y,
	float width, float height,
	const struct cx_color* p_color);

static void cx_console_view_record_text_mesh(
	struct cx_render_draw_command_buffer* p_render_draw_command_buffer,
	const struct cx_gfx_mesh* p_mesh,
	const struct cx_gfx_texture* p_texture,
	float x, float baseline);

void cx_console_view_draw(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_font_render_data,
	const struct cx_gfx_framebuffer* p_fb,
	uint32_t fb_width, uint32_t fb_height,
	const float* p_projection_matrix,
	const float* p_view_matrix) {

	if (!cx_console_view_init()) {
		return;
	}

	cx_console_view_generate_text_meshes(p_console, p_font_render_data);

	struct {
		float projection_matrix[16];
		float view_matrix[16];
	} camera;
	matrix_copy(p_projection_matrix, camera.projection_matrix);
	matrix_copy(p_view_matrix, camera.view_matrix);

	const float margin_x = 5;
	const float margin_y = 5;
	const float padding_x = 5;
	const float padding_y = 3;
	const float line_height = (float)p_font_render_data->p_font->max_glyph_height_;
	const float spacing = 5;
	const uint8_t output_line_count = 25;

	const struct cx_color bg_color = { .rgba = { 0, 0, 0, 0.5f } };
	const struct cx_color fg_color = { .rgba = { 1, 1, 1, 1.0f } };

	// quads

	const float left = margin_x;
	const float bottom = margin_y;
	
	const float input_bg_width = (float)fb_width - margin_x * 2;
	const float input_bg_height = line_height + padding_y * 2;
	const float input_bg_x = left + input_bg_width * 0.5f;
	const float input_bg_y = bottom + input_bg_height * 0.5f;

	const float input_text_x = left + padding_x;
	const float input_text_baseline = bottom + padding_y + (float)p_font_render_data->p_font->descent_;
	
	float pre_cursor_text_width;
	float pre_cursor_text_height;
	cx_text_mesher_measure(
		p_console->input.text.p_buf,
		p_console->input.text.cursor_pos,
		p_font_render_data,
		1,
		&pre_cursor_text_width,
		&pre_cursor_text_height);

	const float input_cursor_width = 1;
	const float input_cursor_height = line_height;
	const float input_cursor_x = left + padding_x + pre_cursor_text_width - input_cursor_width * 0.5f;
	const float input_cursor_y = bottom + padding_y + input_cursor_height * 0.5f;

	const float output_bg_bottom = input_bg_y + input_bg_height * 0.5f + spacing;
	const float output_bg_width = input_bg_width;
	const float output_bg_height = line_height * output_line_count + padding_y * 2;
	const float output_bg_x = input_bg_x;
	const float output_bg_y = output_bg_bottom + 0.5f * output_bg_height;

	const float output_text_x = left + padding_x;
	const float output_text_baseline = output_bg_bottom + padding_y + (float)p_font_render_data->p_font->descent_;

	struct cx_gfx_shader_program_input_block render_pass_shader_program_input_block = {
		.s_name = "blk_camera",
		.p_data = &camera
	};

	struct cx_gfx_render_pass render_pass = {
		.p_framebuffer = p_fb,
		.viewport = { 0, 0, (int32_t)fb_width, (int32_t)fb_height },
		.clear_flags = CX_GFX_RENDER_TARGET_CLEAR_FLAG_depth,
		.clear_depth = 1.0f,
		.pass_input_set = {
			.p_blocks = &render_pass_shader_program_input_block,
			.num_blocks = 1
		}
	};

	struct cx_render_draw_command_buffer render_draw_command_buffer = {
		.p_first = g_render_draw_commands,
		.capacity = CX_ARRAY_LEN(g_render_draw_commands)
	};

	cx_console_view_record_quad(
		&render_draw_command_buffer,
		input_cursor_x,
		input_cursor_y,
		input_cursor_width,
		input_cursor_height,
		&fg_color);
	
	cx_console_view_record_quad(
		&render_draw_command_buffer,
		input_bg_x,
		input_bg_y,
		input_bg_width,
		input_bg_height,
		&bg_color);
	
	cx_console_view_record_quad(
		&render_draw_command_buffer,
		output_bg_x,
		output_bg_y,
		output_bg_width,
		output_bg_height,
		&bg_color);

	cx_console_view_record_text_mesh(&render_draw_command_buffer,
		&g_text_mesh, p_font_render_data->p_glyph_texture, input_text_x, input_text_baseline);

	if (p_console->flogger.ring_entries_.entries_count_ > 0) {
		size_t size;
		const struct cx_flog_entry* p_flog = cx_alloc_ring_get(&p_console->flogger.ring_entries_, 0, &size);
		float log_width, log_height;
		cx_text_mesher_measure(p_flog->s, SIZE_MAX, p_font_render_data, 1, &log_width, &log_height);

		cx_console_view_record_text_mesh(&render_draw_command_buffer,
			&g_log_text_mesh, p_font_render_data->p_glyph_texture,
			output_text_x, output_text_baseline + log_height - line_height);
	}

	cx_gfx_render_pass_execute(&render_pass, render_draw_command_buffer.p_first, render_draw_command_buffer.len);

	g_shader_program_input_texture_pool_len = 0;
	g_shader_program_input_block_pool_len = 0;
	
	cx_gfx_mesh_destroy(&g_text_mesh);
	cx_gfx_mesh_destroy(&g_log_text_mesh);
}

int cx_console_view_init(void) {
	static int b_init;

	if (b_init) {
		return CX_TRUE;
	}

	struct cx_asset_ref asset_ref;

	struct cx_shader* p_shader;

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_flat", &asset_ref);
	p_shader = cx_asset_cache_acquire(&asset_ref);

	g_render_pipeline_flat = (struct cx_render_pipeline) {
		.p_shader = p_shader,
		.state = {
			.cull_mode = CX_CULL_MODE_back
		}
	};

	cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_text", &asset_ref);
	p_shader = cx_asset_cache_acquire(&asset_ref);

	g_render_pipeline_text = (struct cx_render_pipeline) {
		.p_shader = p_shader,
		.state = {
			.cull_mode = CX_CULL_MODE_back
		}
	};

	struct cx_mesh_data quad_mesh_data;
	cx_mesh_gen_quad(0.5f, 0.5f, (float[]){ 0, 0, 1 }, &quad_mesh_data);
	cx_gfx_mesh_create(&quad_mesh_data, CX_GFX_BUFFER_USAGE_static, &g_quad_mesh);
	cx_mesh_gen_free(&quad_mesh_data);

	return (b_init = CX_TRUE);
}

void cx_console_view_generate_text_meshes(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_font_render_data) {
	
	const struct cx_text_mesher_input text_mesher_input = {
		.s_text = p_console->input.text.p_buf,
		.style = {
			.p_font_render_data = p_font_render_data,
			.scale = 1,
			.color = { .rgba = { 0.8f, 0.8f, 0.8f, 1 } }
		}
	};
	
	struct cx_text_mesher_output text_mesher_output;
	size_t num_text_meshes;

	cx_text_mesher_generate(&text_mesher_input, 1, &text_mesher_output, &num_text_meshes);

	cx_gfx_mesh_create(&text_mesher_output.mesh_data, CX_GFX_BUFFER_USAGE_dynamic, &g_text_mesh);

	cx_text_mesher_free(&text_mesher_output, 1);

	if (p_console->flogger.ring_entries_.entries_count_ == 0) {
		return;
	}

	size_t size;
	const struct cx_flog_entry* p_flog = cx_alloc_ring_get(&p_console->flogger.ring_entries_, 0, &size);

	const struct cx_text_mesher_input log_text_mesher_input = {
		.s_text = p_flog->s,
		.style = {
			.p_font_render_data = p_font_render_data,
			.scale = 1,
			.color = { .rgba = { 0.8f, 0.8f, 0.8f, 1 } }
		}
	};

	cx_text_mesher_generate(&log_text_mesher_input, 1, &text_mesher_output, &num_text_meshes);

	cx_gfx_mesh_create(&text_mesher_output.mesh_data, CX_GFX_BUFFER_USAGE_dynamic, &g_log_text_mesh);

	cx_text_mesher_free(&text_mesher_output, 1);
}

void cx_console_view_record_quad(
	struct cx_render_draw_command_buffer* p_render_draw_command_buffer,
	float x, float y,
	float width, float height,
	const struct cx_color* color) {

	float* p_vertex_matrix = g_draw_command_vertex_matrices[p_render_draw_command_buffer->len];
	matrix_make_ts((float[]){ x, y, 0 }, (float[]){ width, height, 1 }, p_vertex_matrix);

	float* p_color = g_draw_command_colors[p_render_draw_command_buffer->len];
	p_color[0] = CX_COLOR_R(*color);
	p_color[1] = CX_COLOR_G(*color);
	p_color[2] = CX_COLOR_B(*color);
	p_color[3] = CX_COLOR_A(*color);

	struct cx_gfx_shader_program_input_block* p_material_shader_program_input_block =
		&g_shader_program_input_block_pool[g_shader_program_input_block_pool_len++];
	p_material_shader_program_input_block->s_name = "blk_material";
	p_material_shader_program_input_block->p_data = p_color;

	struct cx_gfx_shader_program_input_block* p_draw_shader_program_input_block =
		&g_shader_program_input_block_pool[g_shader_program_input_block_pool_len++];
	p_draw_shader_program_input_block->s_name = "blk_object";
	p_draw_shader_program_input_block->p_data = p_vertex_matrix;

	cx_render_draw_command_buffer_push(p_render_draw_command_buffer, &(struct cx_render_draw_command) {
		.pipeline = g_render_pipeline_flat,
		.p_mesh = &g_quad_mesh,
		.material_input_set = {
			.p_blocks = p_material_shader_program_input_block,
			.num_blocks = 1
		},
		.draw_input_set = {
			.p_blocks = p_draw_shader_program_input_block,
			.num_blocks = 1
		}
	});
}

void cx_console_view_record_text_mesh(
	struct cx_render_draw_command_buffer* p_render_draw_command_buffer,
	const struct cx_gfx_mesh* p_mesh,
	const struct cx_gfx_texture* p_texture,
	float x, float baseline) {

	float* p_vertex_matrix = g_draw_command_vertex_matrices[p_render_draw_command_buffer->len];

	matrix_make_translation(x, baseline, 1, p_vertex_matrix);

	struct cx_gfx_shader_program_input_texture* p_material_shader_program_input_texture =
		&g_shader_program_input_texture_pool[g_shader_program_input_texture_pool_len++];
	p_material_shader_program_input_texture->s_name = "u_texture_albedo";
	p_material_shader_program_input_texture->p_texture = p_texture;

	struct cx_gfx_shader_program_input_block* p_draw_shader_program_input_block =
		&g_shader_program_input_block_pool[g_shader_program_input_block_pool_len++];
	p_draw_shader_program_input_block->s_name = "blk_object";
	p_draw_shader_program_input_block->p_data = p_vertex_matrix;

	cx_render_draw_command_buffer_push(p_render_draw_command_buffer, &(struct cx_render_draw_command) {
		.pipeline = g_render_pipeline_text,
		.p_mesh = p_mesh,
		.material_input_set = {
			.p_textures = p_material_shader_program_input_texture,
			.num_textures = 1
		},
		.draw_input_set = {
			.p_blocks = p_draw_shader_program_input_block,
			.num_blocks = 1
		}
	});
}
