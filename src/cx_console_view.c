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
static struct cx_gfx_mesh quad_mesh;

static int cx_console_view_init(void);

void cx_console_view_draw(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_font_render_data,
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

	const struct cx_text_mesh_desc desc = {
		.s_text = p_console->input.input_buf,
		.scale = 1,
		.color = { .rgba = { 0.8f, 0.8f, 0.8f, 1 } },
		.font_data = *p_font_render_data,
	};
	
	struct mesh_primitive text_mesh_primitive;
	cx_text_mesher_generate(&desc, 1, &text_mesh_primitive);

	cx_gfx_mesh_create(&text_mesh, &text_mesh_primitive);
	cx_text_mesher_free(&text_mesh_primitive);

	float transform[16];
	
	const float bg_left = 5;
	const float bg_bottom = 5;
	const float padding_x = 2;
	const float padding_y = 2;
	const float chars_per_line = 70;

	const float bg_width = p_font_render_data->p_font->max_glyph_width_ * chars_per_line + (padding_x * 2);
	const float bg_height = p_font_render_data->p_font->max_glyph_height_ + (padding_y * 2);
	const float bg_x = bg_left + (int)(bg_width * 0.5f);
	const float bg_y = bg_bottom + (int)(bg_height * 0.5f);

	const float text_x = bg_left + padding_x;
	const float text_baseline = bg_bottom + padding_y + p_font_render_data->p_font->descent_;
	
	float pre_cursor_text_width;
	float pre_cursor_text_height;
	cx_text_mesher_measure(
		p_console->input.input_buf,
		p_console->input.text.cursor_pos,
		p_font_render_data,
		1,
		&pre_cursor_text_width,
		&pre_cursor_text_height);

	const float cursor_width = 1;
	const float cursor_height = p_font_render_data->p_font->max_glyph_height_;
	const float cursor_x = text_x + pre_cursor_text_width - (cursor_width * 0.5f);
	const float cursor_y = bg_bottom + padding_y + cursor_height * 0.5f;

	const struct cx_color_f32 bg_color = { .rgba = { 0, 0, 0, 0.5f } };

	// quads

	cx_gfx_program_bind(&program_flat);
	cx_gfx_program_param_block_bind_buffer(&program_flat_pblock_camera, &program_flat_pbuffer_camera, 0, 0);
	cx_gfx_program_param_block_bind_buffer(&program_flat_pblock_object, &program_flat_pbuffer_object, 0, 0);
	cx_gfx_program_param_block_bind_buffer(&program_flat_pblock_mtl, &program_flat_pbuffer_mtl, 0, 0);
	
	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_camera, 0, 0, &camera);
	
	// background

	matrix_make_ts((float[]){ bg_x, bg_y, 0 }, (float[]){ bg_width, bg_height, 1 }, transform);

	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_object, 0, 0, transform);
	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_mtl, 0, 0, &bg_color);

	cx_gfx_mesh_draw(&quad_mesh);
	
	// cursor

	matrix_make_ts((float[]){ cursor_x, cursor_y, 1 }, (float[]){ cursor_width, cursor_height, 1 }, transform);

	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_object, 0, 0, transform);
	cx_gfx_program_param_buffer_set(&program_flat_pbuffer_mtl, 0, 0, &desc.color);

	cx_gfx_mesh_draw(&quad_mesh);
	
	// draw input
	
	matrix_make_translation(text_x, text_baseline, 1, transform);

	cx_gfx_program_bind(&program_text);
	cx_gfx_program_param_block_bind_buffer(&program_text_pblock_camera, &program_text_pbuffer_camera, 0, 0);
	cx_gfx_program_param_block_bind_buffer(&program_text_pblock_object, &program_text_pbuffer_object, 0, 0);
	
	cx_gfx_program_param_buffer_set(&program_text_pbuffer_camera, 0, 0, &camera);
	
	cx_gfx_program_opaque_param_bind_resource(&program_text_opaque_texture_atlas, p_font_render_data->p_glyph_texture);

	cx_gfx_program_param_buffer_set(&program_text_pbuffer_object, 0, 0, transform);
	
	cx_gfx_mesh_draw(&text_mesh);

	cx_gfx_mesh_destroy(&text_mesh);
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
