#include "cx_dbg.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_program.h"
#include "cx_gfx_texture.h"
#include "cx_io.h"
#include "cx_object_id_capturer.h"
#include "cx_pixel_format.h"
#include "gl.h"
#include "math_utils.h"
#include "matrix.h"
#include <stdint.h>

static struct cx_gfx_program program;
static struct cx_gfx_program_param_block program_pblk_camera;
static struct cx_gfx_program_param_block program_pblk_object;
static struct cx_gfx_program_param_buffer program_pbuf_camera;
static struct cx_gfx_program_param_buffer program_pbuf_object;
static struct cx_render_pass render_pass;

static void cx_object_id_capturer_init_statics(void);
static void cx_object_id_capturer_destroy_framebuffer(struct cx_object_id_capturer* p_capturer);
static void cx_object_id_capturer_rebuild_framebuffer(
	struct cx_object_id_capturer* p_capturer,
	uint32_t fb_width,
	uint32_t fb_height);

void cx_object_id_capturer_free(struct cx_object_id_capturer* p_capturer) {
	cx_object_id_capturer_destroy_framebuffer(p_capturer);
}

void cx_object_id_capturer_submit(
	struct cx_object_id_capturer* p_capturer,
	const struct cx_object_id_capturer_item* p_item) {

	cx_object_id_capturer_init_statics();

	struct cx_object_id_capturer_object_data* p_object_data =
		p_capturer->object_data + p_capturer->num_render_pass_commands;

	*p_object_data = (struct cx_object_id_capturer_object_data) {
		.id = p_item->id
	};
	matrix_copy(p_item->p_transform, p_object_data->transform);

	p_capturer->render_pass_commands[p_capturer->num_render_pass_commands] = (struct cx_render_pass_command) {
		.p_mesh = p_item->p_mesh,
		.p_object_data = p_object_data
	};

	p_capturer->num_render_pass_commands++;
}

void cx_object_id_capturer_draw(
	struct cx_object_id_capturer* p_capturer,
	const float* p_projection_matrix,
	const float* p_view_matrix,
	uint32_t fb_width,
	uint32_t fb_height) {

	cx_object_id_capturer_init_statics();

    if (p_capturer->framebuffer_width != fb_width ||
		p_capturer->framebuffer_height!= fb_height) {
		cx_object_id_capturer_rebuild_framebuffer(p_capturer, fb_width, fb_height);
    }

	cx_gfx_framebuffer_bind(&p_capturer->framebuffer);
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST); 
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);

	float camera[32];
	matrix_copy(p_projection_matrix, &camera[0]);
	matrix_copy(p_view_matrix, &camera[16]);

	struct cx_render_pass_data render_pass_data = {
		.p_data = camera
	};

	cx_render_pass_execute(
		&render_pass,
		&render_pass_data,
		p_capturer->render_pass_commands,
		p_capturer->num_render_pass_commands);

	p_capturer->num_render_pass_commands = 0;
}

uint32_t cx_object_id_capturer_query(const struct cx_object_id_capturer* p_capturer, float x, float y) {
    x = clampf(x, 0, 1);
	y = clampf(y, 0, 1);
	
	uint32_t pixel_location[] = { 
		(float)p_capturer->framebuffer_width * x,
		(float)p_capturer->framebuffer_height * (1.0f - y)
	};
	unsigned int pixel_value;

	cx_gfx_framebuffer_read(
		&p_capturer->framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
		pixel_location,
		(uint32_t[]){ 1, 1 },
		&pixel_value);

	return pixel_value;
}

void cx_object_id_capturer_destroy_framebuffer(struct cx_object_id_capturer* p_capturer) {
	cx_gfx_framebuffer_destroy(&p_capturer->framebuffer);
	cx_gfx_texture_destroy(&p_capturer->framebuffer_color);
	cx_gfx_texture_destroy(&p_capturer->framebuffer_depth_stencil);
	*p_capturer = (struct cx_object_id_capturer){0};
}

void cx_object_id_capturer_rebuild_framebuffer(
	struct cx_object_id_capturer* p_capturer,
	uint32_t fb_width,
	uint32_t fb_height) {

	cx_object_id_capturer_destroy_framebuffer(p_capturer);	
	
	cx_gfx_texture_create(
		&p_capturer->framebuffer_color,
		fb_width, fb_height,
		CX_PIXEL_FORMAT_red_u32);

	cx_gfx_texture_create(
		&p_capturer->framebuffer_depth_stencil,
		fb_width, fb_height,
		CX_PIXEL_FORMAT_depth_stencil);

	cx_gfx_framebuffer_create(&p_capturer->framebuffer);
	cx_gfx_framebuffer_set_attachment(
		&p_capturer->framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
		&p_capturer->framebuffer_color);
	cx_gfx_framebuffer_set_attachment(
		&p_capturer->framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_depth_stencil,
		&p_capturer->framebuffer_depth_stencil);

	p_capturer->framebuffer_width = fb_width;
	p_capturer->framebuffer_height = fb_height;
}

void cx_object_id_capturer_init_statics(void) {
	static int b_done = 0;
	if (b_done) {
		return;
	}

	void* p_vsource;
	void* p_fsource;
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/object_id.vert", (void**)&p_vsource, 0) == CX_ERROR_none);
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/object_id.frag", (void**)&p_fsource, 0) == CX_ERROR_none);

	struct cx_gfx_program_source program_source = {
		.s_vertex_stage_source = p_vsource,
		.s_fragment_stage_source = p_fsource
	};

	CX_ASSERT(cx_gfx_program_create(&program) == CX_ERROR_none);
	CX_ASSERT(cx_gfx_program_build(&program, &program_source) == CX_ERROR_none);

	cx_io_file_free(p_vsource);
	cx_io_file_free(p_fsource);

	cx_gfx_program_refl_param_block(&program, "blk_camera", &program_pblk_camera);
	cx_gfx_program_refl_param_block(&program, "blk_object", &program_pblk_object);

	cx_gfx_program_param_buffer_create(&program_pbuf_camera, program_pblk_camera.size_);
	cx_gfx_program_param_buffer_create(&program_pbuf_object, program_pblk_object.size_);

	render_pass = (struct cx_render_pass) {
		.p_program = &program,
		.p_pass_block = &program_pblk_camera,
		.p_pass_buffer = &program_pbuf_camera,
		.p_object_block = &program_pblk_object,
		.p_object_buffer = &program_pbuf_object
	};

	b_done = 1;
}
