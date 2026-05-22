#include "cx_console.h"
#include "cx_console_view.h"
#include "cx_font.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"
#include "cx_io.h"
#include "cx_mesh_gen.h"
#include "cx_text_mesher.h"
#include "matrix.h"
#include "mesh.h"

static struct cx_gfx_program program_text;

static struct cx_gfx_program_param_block program_text_pblock_camera;
static struct cx_gfx_program_param_block program_text_pblock_object;
static struct cx_gfx_program_opaque_param program_text_opaque_texture_atlas;

static struct cx_gfx_program_param_buffer program_text_pbuffer_camera;
static struct cx_gfx_program_param_buffer program_text_pbuffer_object;

static struct cx_gfx_program program_flat;

static struct cx_gfx_program_param_block program_flat_pblock_camera;
static struct cx_gfx_program_param_block program_flat_pblock_object;
static struct cx_gfx_program_param_block program_flat_pblock_mtl;

static struct cx_gfx_program_param_buffer program_flat_pbuffer_camera;
static struct cx_gfx_program_param_buffer program_flat_pbuffer_object;
static struct cx_gfx_program_param_buffer program_flat_pbuffer_mtl;

static struct cx_gfx_mesh text_mesh;
static struct cx_gfx_mesh log_text_mesh;
static struct cx_gfx_mesh quad_mesh;

static int cx_console_view_init(void);
static void cx_console_view_generate_text_meshes(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_font_render_data);
static void cx_console_view_draw_quad(
	float x, float y,
	float width, float height,
	const struct cx_color_f32* p_color);
static void cx_console_view_draw_text_mesh(const struct cx_gfx_mesh* p_mesh, float x, float baseline);

void cx_console_view_draw(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_font_render_data,
	unsigned int max_width,
	const float* p_projection_matrix,
	const float* p_view_matrix) {

	if (!cx_console_view_init()) {
		return;
	}


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
	const float line_height = p_font_render_data->p_font->max_glyph_height_;
	const float spacing = 5;
	const uint8_t output_line_count = 25;

	const struct cx_color_f32 bg_color = { .rgba = { 0, 0, 0, 0.5f } };
	const struct cx_color_f32 fg_color = { .rgba = { 1, 1, 1, 1    }};

	cx_console_view_generate_text_meshes(p_console, p_font_render_data);
	
	// quads

	cx_gfx_program_bind(&program_flat);
	cx_gfx_program_param_block_bind_buffer(&program_flat_pblock_camera, &program_flat_pbuffer_camera, 0, 0);
	cx_gfx_program_param_block_bind_buffer(&program_flat_pblock_object, &program_flat_pbuffer_object, 0, 0);
	cx_gfx_program_param_block_bind_buffer(&program_flat_pblock_mtl, &program_flat_pbuffer_mtl, 0, 0);
	
	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_camera, 0, 0, &camera);

	const float left = margin_x;
	const float bottom = margin_y;
	
	const float input_bg_width = max_width - margin_x * 2;
	const float input_bg_height = line_height + padding_y * 2;
	const float input_bg_x = left + input_bg_width * 0.5f;
	const float input_bg_y = bottom + input_bg_height * 0.5f;

	const float input_text_x = left + padding_x;
	const float input_text_baseline = bottom + padding_y + p_font_render_data->p_font->descent_;
	
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
	const float output_text_baseline = output_bg_bottom + padding_y + p_font_render_data->p_font->descent_;

	cx_console_view_draw_quad(input_cursor_x, input_cursor_y, input_cursor_width, input_cursor_height, &fg_color);
	
	cx_console_view_draw_quad(input_bg_x, input_bg_y, input_bg_width, input_bg_height, &bg_color);
	
	cx_console_view_draw_quad(output_bg_x, output_bg_y, output_bg_width, output_bg_height, &bg_color);

	// text

	cx_gfx_program_bind(&program_text);
	cx_gfx_program_param_block_bind_buffer(&program_text_pblock_camera, &program_text_pbuffer_camera, 0, 0);
	cx_gfx_program_param_block_bind_buffer(&program_text_pblock_object, &program_text_pbuffer_object, 0, 0);
	
	cx_gfx_program_param_buffer_set(&program_text_pbuffer_camera, 0, 0, &camera);
	
	cx_gfx_program_opaque_param_bind_resource(&program_text_opaque_texture_atlas, p_font_render_data->p_glyph_texture);

	cx_console_view_draw_text_mesh(&text_mesh, input_text_x, input_text_baseline);

	if (p_console->flogger.ring_entries_.entries_count_ > 0) {
		size_t size;
		const struct cx_flog_entry* p_flog = cx_alloc_ring_get(&p_console->flogger.ring_entries_, 0, &size);
		float log_width, log_height;
		cx_text_mesher_measure(p_flog->s, -1, p_font_render_data, 1, &log_width, &log_height);

		cx_console_view_draw_text_mesh(&log_text_mesh, output_text_x, output_text_baseline + log_height - line_height);
	}
	
	cx_gfx_mesh_destroy(&log_text_mesh);
}

int cx_console_view_init(void) {
	static int b_init;

	if (b_init) {
		return 1;
	}

	size_t len;
	char* s_vert;
	char* s_frag;
	struct cx_gfx_program_source source;

	cx_io_file_read_all("res/builtin/shd/text.vert", (void**)&s_vert, &len);
	cx_io_file_read_all("res/builtin/shd/text.frag", (void**)&s_frag, &len);

	source.s_vertex_stage_source = s_vert;
	source.s_fragment_stage_source = s_frag;

	cx_gfx_program_create(&program_text);
	cx_gfx_program_build(&program_text, &source);

	cx_io_file_free(s_vert);
	cx_io_file_free(s_frag);

	cx_gfx_program_refl_param_block(&program_text, "blk_camera", &program_text_pblock_camera);
	cx_gfx_program_refl_param_block(&program_text, "blk_object", &program_text_pblock_object);
	cx_gfx_program_refl_opaque_param(&program_text, "u_texture_albedo", &program_text_opaque_texture_atlas);

	cx_gfx_program_param_buffer_create(&program_text_pbuffer_camera, program_text_pblock_camera.size_);
	cx_gfx_program_param_buffer_create(&program_text_pbuffer_object, program_text_pblock_object.size_);
	
	cx_io_file_read_all("res/builtin/shd/flat.vert", (void**)&s_vert, &len);
	cx_io_file_read_all("res/builtin/shd/flat.frag", (void**)&s_frag, &len);

	source.s_vertex_stage_source = s_vert;
	source.s_fragment_stage_source = s_frag;

	cx_gfx_program_create(&program_flat);
	cx_gfx_program_build(&program_flat, &source);

	cx_io_file_free(s_vert);
	cx_io_file_free(s_frag);

	cx_gfx_program_refl_param_block(&program_flat, "blk_camera", &program_flat_pblock_camera);
	cx_gfx_program_refl_param_block(&program_flat, "blk_object", &program_flat_pblock_object);
	cx_gfx_program_refl_param_block(&program_flat, "blk_material_properties", &program_flat_pblock_mtl);

	cx_gfx_program_param_buffer_create(&program_flat_pbuffer_camera, program_flat_pblock_camera.size_);
	cx_gfx_program_param_buffer_create(&program_flat_pbuffer_object, program_flat_pblock_object.size_);
	cx_gfx_program_param_buffer_create(&program_flat_pbuffer_mtl, program_flat_pblock_mtl.size_);

	struct mesh_primitive quad_mesh_prim;
	cx_mesh_gen_quad(0.5f, 0.5f, (float[]){ 0, 0, 1 }, &quad_mesh_prim);
	cx_gfx_mesh_create(&quad_mesh, &quad_mesh_prim);
	cx_mesh_gen_free(&quad_mesh_prim);

	return (b_init = 1);
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

	cx_gfx_mesh_create(&text_mesh, &text_mesher_output.primitive);
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

	cx_gfx_mesh_create(&log_text_mesh, &text_mesher_output.primitive);
	cx_text_mesher_free(&text_mesher_output, 1);
}

void cx_console_view_draw_quad(
	float x, float y,
	float width, float height,
	const struct cx_color_f32* p_color) {
	float t[16];

	matrix_make_ts((float[]){ x, y, 0 }, (float[]){ width, height, 1 }, t);

	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_object, 0, 0, t);
	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_mtl, 0, 0, p_color);

	cx_gfx_mesh_draw(&quad_mesh);
}

void cx_console_view_draw_text_mesh(const struct cx_gfx_mesh* p_mesh, float x, float baseline) {
	float transform[16];
	matrix_make_translation(x, baseline, 1, transform);

	cx_gfx_program_param_buffer_set(&program_text_pbuffer_object, 0, 0, transform);
	
	cx_gfx_mesh_draw(p_mesh);
}
