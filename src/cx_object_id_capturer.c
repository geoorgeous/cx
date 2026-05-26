#include "cx_dbg.h"
#include "cx_gfx_framebuffer.h"
#include "cx_gfx_texture.h"
#include "cx_io.h"
#include "cx_object_id_capturer.h"
#include "cx_pixel_format.h"
#include "gl.h"
#include "math_utils.h"
#include "matrix.h"
#include <stdint.h>

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

void cx_object_id_capturer_draw(
	struct cx_object_id_capturer* p_capturer,
	const float* p_projection_matrix,
	const float* p_view_matrix,
	uint32_t fb_width,
	uint32_t fb_height,
	const struct cx_render_command_buffer* p_render_command_buffer) {

	cx_object_id_capturer_init_statics();

    if (p_capturer->framebuffer_width != fb_width ||
		p_capturer->framebuffer_height!= fb_height) {
		cx_object_id_capturer_rebuild_framebuffer(p_capturer, fb_width, fb_height);
    }

	struct cx_render_pass_execute_info render_pass_execute_info = {
		.p_framebuffer = &p_capturer->framebuffer,
		.viewport = { 0, 0, fb_width, fb_height },
		.b_clear_color = 1,
		.b_clear_depth = 1,
		.clear_depth = 1.0f
	};

	float camera[32];
	matrix_copy(p_projection_matrix, &camera[0]);
	matrix_copy(p_view_matrix, &camera[16]);

	struct cx_render_pass_data render_pass_data = {
		.p_data = camera
	};

	cx_render_pass_execute(
		&render_pass,
		&render_pass_execute_info,
		&render_pass_data,
		p_render_command_buffer);
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

	const struct cx_render_pass_build_info render_pass_build_info = {
		.program_source = {
			.s_vertex_stage_source = p_vsource,
			.s_fragment_stage_source = p_fsource
		},
		.s_pass_block_name = "blk_camera",
		.s_object_block_name = "blk_object",
	};

	CX_ASSERT(cx_render_pass_build(&render_pass_build_info, &render_pass));

	cx_io_file_free(p_vsource);
	cx_io_file_free(p_fsource);

	b_done = 1;
}
