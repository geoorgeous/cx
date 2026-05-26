#include "cx_console.h"
#include "cx_console_view.h"
#include "cx_dbg.h"
#include "cx_font.h"
#include "cx_gfx_mesh.h"
#include "cx_render_pass.h"
#include "cx_io.h"
#include "cx_mesh_gen.h"
#include "cx_text_mesher.h"
#include "matrix.h"
#include "mesh.h"

#define CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS 1024

static struct cx_render_pass render_pass_flat_color;
static struct cx_render_pass render_pass_text;

static struct cx_render_command render_pass_commands[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS];
static float render_command_object_data[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS][16];
static struct cx_color_f32 render_command_material_data[CX_CONSOLE_VIEW_MAX_RENDER_COMMANDS];

static struct cx_gfx_mesh text_mesh;
static struct cx_gfx_mesh log_text_mesh;
static struct cx_gfx_mesh quad_mesh;

static int cx_console_view_init(void);

static void cx_console_view_generate_text_meshes(
	const struct cx_console* p_console,
	const struct cx_font_render_data* p_font_render_data);

static void cx_console_view_record_quad(
	struct cx_render_command_buffer* p_render_command_buffer,
	float x, float y,
	float width, float height,
	const struct cx_color_f32* p_color);

static void cx_console_view_record_text_mesh(
	struct cx_render_command_buffer* p_render_command_buffer,
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
	const float line_height = p_font_render_data->p_font->max_glyph_height_;
	const float spacing = 5;
	const uint8_t output_line_count = 25;

	const struct cx_color_f32 bg_color = { .rgba = { 0, 0, 0, 0.5f } };
	const struct cx_color_f32 fg_color = { .rgba = { 1, 1, 1, 1.0f } };

	// quads

	const float left = margin_x;
	const float bottom = margin_y;
	
	const float input_bg_width = fb_width - margin_x * 2;
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

	struct cx_render_pass_execute_info render_pass_execute_info = {
		.p_framebuffer = p_fb,
		.viewport = { 0, 0, fb_width, fb_height },
		.b_clear_depth = 1,
		.clear_depth = 1.0f
	};

	struct cx_render_pass_data render_pass_data = {
		.p_data = &camera
	};

	struct cx_render_command_buffer render_command_buffer = {
		.p_commands = render_pass_commands,
		.capacity = CX_ARRAY_LEN(render_pass_commands)
	};

	// quads

	cx_console_view_record_quad(&render_command_buffer,
		input_cursor_x, input_cursor_y, input_cursor_width, input_cursor_height, &fg_color);
	
	cx_console_view_record_quad(&render_command_buffer,
		input_bg_x, input_bg_y, input_bg_width, input_bg_height, &bg_color);
	
	cx_console_view_record_quad(&render_command_buffer,
		output_bg_x, output_bg_y, output_bg_width, output_bg_height, &bg_color);

	cx_render_pass_execute(
		&render_pass_flat_color,
		&render_pass_execute_info,
		&render_pass_data,
		&render_command_buffer);
	render_command_buffer.num = 0;

	// text

	cx_console_view_record_text_mesh(&render_command_buffer,
		&text_mesh, p_font_render_data->p_glyph_texture, input_text_x, input_text_baseline);

	if (p_console->flogger.ring_entries_.entries_count_ > 0) {
		size_t size;
		const struct cx_flog_entry* p_flog = cx_alloc_ring_get(&p_console->flogger.ring_entries_, 0, &size);
		float log_width, log_height;
		cx_text_mesher_measure(p_flog->s, -1, p_font_render_data, 1, &log_width, &log_height);

		cx_console_view_record_text_mesh(&render_command_buffer,
			&log_text_mesh, p_font_render_data->p_glyph_texture,
			output_text_x, output_text_baseline + log_height - line_height);
	}

	render_pass_execute_info.b_clear_depth = 0;

	cx_render_pass_execute(
		&render_pass_text,
		&render_pass_execute_info,
		&render_pass_data,
		&render_command_buffer);
	render_command_buffer.num = 0;
	
	cx_gfx_mesh_destroy(&text_mesh);
	cx_gfx_mesh_destroy(&log_text_mesh);
}

int cx_console_view_init(void) {
	static int b_init;

	if (b_init) {
		return 1;
	}

	char* s_vert;
	char* s_frag;

	cx_io_file_read_all("res/builtin/shd/flat.vert", (void**)&s_vert, CX_NULL);
	cx_io_file_read_all("res/builtin/shd/flat.frag", (void**)&s_frag, CX_NULL);

	CX_ASSERT(cx_render_pass_build(&((struct cx_render_pass_build_info){
		.program_source = {
			.s_vertex_stage_source = s_vert,
			.s_fragment_stage_source = s_frag
		},
		.s_pass_block_name = "blk_camera",
		.s_object_block_name = "blk_object",
		.s_material_block_name = "blk_material_properties"
	}), &render_pass_flat_color));

	cx_io_file_free(s_vert);
	cx_io_file_free(s_frag);
	
	cx_io_file_read_all("res/builtin/shd/text.vert", (void**)&s_vert, CX_NULL);
	cx_io_file_read_all("res/builtin/shd/text.frag", (void**)&s_frag, CX_NULL);

	CX_ASSERT(cx_render_pass_build(&((struct cx_render_pass_build_info){
		.program_source = {
			.s_vertex_stage_source = s_vert,
			.s_fragment_stage_source = s_frag
		},
		.s_pass_block_name = "blk_camera",
		.s_object_block_name = "blk_object",
		.p_s_opaque_param_names = (const char*[]){ "u_texture_albedo" },
		.num_opaque_params = 1
	}), &render_pass_text));

	cx_io_file_free(s_vert);
	cx_io_file_free(s_frag);

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

void cx_console_view_record_quad(
	struct cx_render_command_buffer* p_render_command_buffer,
	float x, float y,
	float width, float height,
	const struct cx_color_f32* color) {

	matrix_make_ts((float[]){ x, y, 0 }, (float[]){ width, height, 1 },
		render_command_object_data[p_render_command_buffer->num]);

	render_command_material_data[p_render_command_buffer->num] = *color;

	cx_render_command_buffer_push(p_render_command_buffer, &((struct cx_render_command){
		.p_mesh = &quad_mesh,
		.p_object_data = &render_command_object_data[p_render_command_buffer->num],
		.p_material_data = &render_command_material_data[p_render_command_buffer->num]
	}));
}

void cx_console_view_record_text_mesh(
	struct cx_render_command_buffer* p_render_command_buffer,
	const struct cx_gfx_mesh* p_mesh,
	const struct cx_gfx_texture* p_texture,
	float x, float baseline) {

	matrix_make_translation(x, baseline, 1,
		render_command_object_data[p_render_command_buffer->num]);

	cx_render_command_buffer_push(p_render_command_buffer, &((struct cx_render_command){
		.p_mesh = p_mesh,
		.p_object_data = &render_command_object_data[p_render_command_buffer->num],
		.p_opaque_resources = { p_texture },
		.num_opaque_params = 1
	}));
}
