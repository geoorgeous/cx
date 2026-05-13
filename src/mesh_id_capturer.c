#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"
#include "cx_gfx_texture.h"
#include "cx_pixel_format.h"
#include "gl.h"
#include "math_utils.h"
#include "matrix.h"
#include <stdint.h>
#include "mesh_id_capturer.h"

static struct cx_gfx_program program;
static struct cx_gfx_program_param_block program_pblk_camera;
static struct cx_gfx_program_param_block program_pblk_object;
static struct cx_gfx_program_param_buffer program_pbuf_camera;
static struct cx_gfx_program_param_buffer program_pbuf_object;

static void mesh_id_capturer_destroy_framebuffer(struct mesh_id_capturer* p_mesh_id_capturer);

void mesh_id_capturer_free(struct mesh_id_capturer* p_mesh_id_capturer) {
	mesh_id_capturer_destroy_framebuffer(p_mesh_id_capturer);
	*p_mesh_id_capturer = (struct mesh_id_capturer){0};
}

void mesh_id_capturer_begin(
	struct mesh_id_capturer* p_mesh_id_capturer,
	uint32_t framebuffer_width, uint32_t framebuffer_height,
	const float* p_projection_matrix,
	const float* p_view_matrix) {

    if (!cx_gfx_program_is_built(&program)) {
		struct cx_gfx_program_source program_source = {
			.s_vertex_stage_source = "#version 330 core\n"
				"layout(std140) uniform blk_camera {"
					"mat4 u_projection_matrix;"
					"mat4 u_view_matrix;"
				"};"
				"layout(std140) uniform blk_object {"
					"mat4 u_vertex_matrix;"
					"uint u_object_id;"
				"};"
				"layout(location=0) in vec3 a_pos;"
				"out uint v_object_id;"
				"void main() {"
					"v_object_id = u_object_id;"
					"gl_Position = u_projection_matrix * u_view_matrix * u_vertex_matrix * vec4(a_pos, 1.0);"
				"}",
			.s_fragment_stage_source = "#version 330 core\n"
				"layout(std140) uniform blk_object {"
					"mat4 u_vertex_matrix;"
					"uint u_object_id;"
				"};"
				"out uint f_color;"
				"void main() {"
					"f_color = u_object_id;"
				"}"
		};

		cx_gfx_program_create(&program);
		cx_gfx_program_build(&program, &program_source);

		if (!cx_gfx_program_is_built(&program)) {
			return;
		}
		
		cx_gfx_program_refl_param_block(&program, "blk_camera", &program_pblk_camera);
		cx_gfx_program_refl_param_block(&program, "blk_object", &program_pblk_object);

		cx_gfx_program_param_buffer_create(&program_pbuf_camera, program_pblk_camera.size_);
		cx_gfx_program_param_buffer_create(&program_pbuf_object, program_pblk_object.size_);
    }

    if (p_mesh_id_capturer->framebuffer_width != framebuffer_width ||
		p_mesh_id_capturer->framebuffer_height!= framebuffer_height) {

		mesh_id_capturer_destroy_framebuffer(p_mesh_id_capturer);	
        
		cx_gfx_texture_create(
			&p_mesh_id_capturer->framebuffer_color,
			framebuffer_width, framebuffer_height,
			CX_PIXEL_FORMAT_red_u32);

		cx_gfx_texture_create(
			&p_mesh_id_capturer->framebuffer_depth_stencil,
			framebuffer_width, framebuffer_height,
			CX_PIXEL_FORMAT_depth_stencil);

		cx_gfx_framebuffer_create(&p_mesh_id_capturer->framebuffer);
		cx_gfx_framebuffer_set_attachment(
			&p_mesh_id_capturer->framebuffer,
			CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
			&p_mesh_id_capturer->framebuffer_color);
		cx_gfx_framebuffer_set_attachment(
			&p_mesh_id_capturer->framebuffer,
			CX_GFX_FRAMEBUFFER_ATTACHMENT_depth_stencil,
			&p_mesh_id_capturer->framebuffer_depth_stencil);

        p_mesh_id_capturer->framebuffer_width = framebuffer_width;
        p_mesh_id_capturer->framebuffer_height = framebuffer_height;
    }

	cx_gfx_framebuffer_bind(&p_mesh_id_capturer->framebuffer);
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST); 
    glViewport(0, 0, (GLsizei)framebuffer_width, (GLsizei)framebuffer_height);

	cx_gfx_program_bind(&program);

	cx_gfx_program_param_block_bind_buffer(&program_pblk_camera, &program_pbuf_camera, 0, 0);
	cx_gfx_program_param_block_bind_buffer(&program_pblk_object, &program_pbuf_object, 0, 0);

	struct {
		float projection_matrix[16];
		float view_matrix[16];
	} camera;

	matrix_copy(p_projection_matrix, camera.projection_matrix);
	matrix_copy(p_view_matrix, camera.view_matrix);

	cx_gfx_program_param_buffer_set(&program_pbuf_camera, 0, 0, &camera);
}

void mesh_id_capturer_draw_item(const struct mesh_id_capturer_item* p_item) {
	struct {
		float        vertex_matrix[16];
		unsigned int id;
	} object = {
		.id = p_item->id
	};

	matrix_copy(p_item->p_transform, object.vertex_matrix);

	cx_gfx_program_param_buffer_set(&program_pbuf_object, 0, 0, &object);

	cx_gfx_mesh_draw(p_item->p_mesh);
}

unsigned int mesh_id_capturer_query(const struct mesh_id_capturer* p_mesh_id_capturer, float x, float y) {
    x = clampf(x, 0, 1);
	y = clampf(y, 0, 1);
	
	uint32_t pixel_location[] = { 
		(float)p_mesh_id_capturer->framebuffer_width * x,
		(float)p_mesh_id_capturer->framebuffer_height * (1.0f - y)
	};
	unsigned int pixel_value;

	cx_gfx_framebuffer_read(
		&p_mesh_id_capturer->framebuffer,
		CX_GFX_FRAMEBUFFER_ATTACHMENT_color0,
		pixel_location,
		(uint32_t[]){ 1, 1 },
		&pixel_value);

	return pixel_value;
}

void mesh_id_capturer_destroy_framebuffer(struct mesh_id_capturer* p_mesh_id_capturer) {
	cx_gfx_framebuffer_destroy(&p_mesh_id_capturer->framebuffer);
	cx_gfx_texture_destroy(&p_mesh_id_capturer->framebuffer_color);
	cx_gfx_texture_destroy(&p_mesh_id_capturer->framebuffer_depth_stencil);
	*p_mesh_id_capturer = (struct mesh_id_capturer){0};
}
