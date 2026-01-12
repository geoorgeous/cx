#include "cx_gfx_framebuffer.h"
#include "cx_gfx_mesh.h"
#include "cx_gfx_program.h"
#include "cx_gfx_texture.h"
#include "cx_pixel_format.h"
#include "gl.h"
#include "matrix.h"
#include <stdint.h>
#include "mesh_id_capturer.h"

static struct cx_gfx_program program;
static struct cx_gfx_program_param_block program_pblk_camera;
static struct cx_gfx_program_param_block program_pblk_object;
static struct cx_gfx_program_param_buffer program_pbuf_camera;
static struct cx_gfx_program_param_buffer program_pbuf_object;

void mesh_id_capturer_free_resources(struct mesh_id_capturer* p_mesh_id_capturer) {
	cx_gfx_framebuffer_destroy(&p_mesh_id_capturer->framebuffer);
	cx_gfx_texture_destroy(&p_mesh_id_capturer->framebuffer_color);
	cx_gfx_texture_destroy(&p_mesh_id_capturer->framebuffer_depth_stencil);
	*p_mesh_id_capturer = (struct mesh_id_capturer){0};
}

void mesh_id_capturer_begin(struct mesh_id_capturer* p_mesh_id_capturer, const uint32_t* p_framebuffer_size, const float* p_projection_matrix, const float* p_view_matrix) {
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
		
		cx_gfx_program_refl_param_block(&program, "blk_camera", &program_pblk_camera);
		cx_gfx_program_refl_param_block(&program, "blk_object", &program_pblk_object);

		cx_gfx_program_param_buffer_create(&program_pbuf_camera, program_pblk_camera._size);
		cx_gfx_program_param_buffer_create(&program_pbuf_object, program_pblk_object._size);
    }

	if (!cx_gfx_program_is_built(&program)) {
		return;
	}

    if (p_mesh_id_capturer->framebuffer_size[0] != p_framebuffer_size[0] ||
		p_mesh_id_capturer->framebuffer_size[1] != p_framebuffer_size[1]) {

		mesh_id_capturer_free_resources(p_mesh_id_capturer);	
        
		cx_gfx_texture_create(&p_mesh_id_capturer->framebuffer_color, p_framebuffer_size, CX_PIXEL_FORMAT_red_u32); // GL_RED_INTEGER, GL_UNSIGNED_INT

		cx_gfx_texture_create(&p_mesh_id_capturer->framebuffer_depth_stencil, p_framebuffer_size, CX_PIXEL_FORMAT_depth_stencil);

		cx_gfx_framebuffer_create(&p_mesh_id_capturer->framebuffer);
		cx_gfx_framebuffer_set_attachment(&p_mesh_id_capturer->framebuffer, CX_GFX_FRAMEBUFFER_ATTACHMENT_color0, &p_mesh_id_capturer->framebuffer_color);
		cx_gfx_framebuffer_set_attachment(&p_mesh_id_capturer->framebuffer, CX_GFX_FRAMEBUFFER_ATTACHMENT_depth_stencil, &p_mesh_id_capturer->framebuffer_depth_stencil);

        p_mesh_id_capturer->framebuffer_size[0] = p_framebuffer_size[0];
        p_mesh_id_capturer->framebuffer_size[1] = p_framebuffer_size[1];
    }

	cx_gfx_framebuffer_bind(&p_mesh_id_capturer->framebuffer);
    
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST); 
    glViewport(0, 0, (GLsizei)p_framebuffer_size[0], (GLsizei)p_framebuffer_size[1]);

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

void mesh_id_capturer_submit(const struct cx_gfx_mesh* p_mesh, const float* p_transform, unsigned int id) {
	struct {
		float        vertex_matrix[16];
		unsigned int id;
	} object;

	matrix_copy(p_transform, object.vertex_matrix);
	object.id = id;

	cx_gfx_program_param_buffer_set(&program_pbuf_object, 0, 0, &object);

    cx_gfx_mesh_draw(p_mesh);
}

unsigned int mesh_id_capturer_query(const struct mesh_id_capturer* p_mesh_id_capturer, const float* p_normalized_coordinates) {
    if (p_normalized_coordinates[0] < 0 ||
		p_normalized_coordinates[1] < 0 ||
		p_normalized_coordinates[0] > 1 ||
		p_normalized_coordinates[1] > 1) {
        
		return 0;
    }

	uint32_t pixel_location[] = { 
		(float)p_mesh_id_capturer->framebuffer_size[0] * p_normalized_coordinates[0],
		(float)p_mesh_id_capturer->framebuffer_size[1] * (1.0f - p_normalized_coordinates[1])
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
