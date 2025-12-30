#include <stdlib.h>

#include "cx_built_in_assets.h"
#include "darr.h"
#include "dev_draw.h"
#include "dev.h"
#include "gl_mesh.h"
#include "gl_program.h"
#include "gl.h"
#include "logging.h"
#include "matrix.h"
#include "mesh_factory.h"
#include "mesh.h"
#include "vector.h"

#define SPHERE_MESH_RAD 0.5f
#define SPHERE_MESH_DIV 12

#define CAPSULE_MESH_RAD 0.25f
#define CAPSULE_MESH_MID_HEIGHT 1.0f

struct draw_command {
	const struct gl_mesh* p_gl_mesh;
	float                 transform[16];
	u32_r8g8b8a8          color;
	float                 remaining_duration;
};

static int draw_command_cmp(const struct draw_command* p_a, const struct draw_command* p_b);

static struct {
	struct gl_program program;
	struct gl_program_uniform u_proj_mat;
	struct gl_program_uniform u_view_mat;
	struct gl_program_uniform u_modl_mat;
	struct gl_program_uniform u_color;
	struct darr       line_draw_commands;
	struct darr       fill_draw_commands;
} dev_draw_state;

static void dev_draw_init(void);
static void dev_draw_mesh_internal(const struct gl_mesh* p_gl_mesh, const float* p_transform, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration);
static void dev_draw_flush_draw_commands(struct darr* p_draw_commands, float delta_time);

void dev_draw_mesh(const struct gl_mesh* p_gl_mesh, const float* p_transform, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration) {
	ENSURE_DEV_MODE();

	dev_draw_init();

	dev_draw_mesh_internal(p_gl_mesh, p_transform, line_color, fill_color, duration);
}

void dev_draw_sphere(const float* p_center, float radius, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration) {
	ENSURE_DEV_MODE();

	dev_draw_init();

	float transform[16];

	matrix_make_uniform_scale(radius * 2, transform);

	transform[12] = p_center[0];
	transform[13] = p_center[1];
	transform[14] = p_center[2];

	dev_draw_mesh_internal((struct gl_mesh*)cx_built_in_assets_get(CX_BUILT_IN_ASSET_mesh_sphere), transform, line_color, fill_color, duration);
}

void dev_draw_capsule(const float* p_p0, const float* p_p1, float radius, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration) {
	ENSURE_DEV_MODE();

	dev_draw_init();

	float transform_cap0[16];
	float transform_cap1[16];
	float transform_mid[16];

	// todo: transforms

	dev_draw_mesh((struct gl_mesh*)cx_built_in_assets_get(CX_BUILT_IN_ASSET_mesh_capsule_cap), transform_cap0, line_color, fill_color, duration);
	dev_draw_mesh((struct gl_mesh*)cx_built_in_assets_get(CX_BUILT_IN_ASSET_mesh_capsule_cap), transform_cap1, line_color, fill_color, duration);
	dev_draw_mesh((struct gl_mesh*)cx_built_in_assets_get(CX_BUILT_IN_ASSET_mesh_capsule_mid), transform_mid, line_color, fill_color, duration);
}

void dev_draw_box(const float* p_min, const float* p_max, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration) {
	ENSURE_DEV_MODE();

	dev_draw_init();

	float transform[16];

	// todo: transform

	dev_draw_mesh((struct gl_mesh*)cx_built_in_assets_get(CX_BUILT_IN_ASSET_mesh_cube), transform, line_color, fill_color, duration);
}

void dev_draw_plane(const float* p_normal, float distance, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration) {
	ENSURE_DEV_MODE();

	dev_draw_init();
	
	float transform[16];

	// todo: transform

	dev_draw_mesh((struct gl_mesh*)cx_built_in_assets_get(CX_BUILT_IN_ASSET_mesh_plane), transform, line_color, fill_color, duration);
}

void dev_draw_line(const float* p_p0, const float* p_p1, u32_r8g8b8a8 color, float duration) {
	ENSURE_DEV_MODE();

	dev_draw_init();

	// todo
}

void dev_draw_flush(const float* p_projection_matrix, const float* p_view_matrix, float delta_time) {
	if (dev_draw_state.line_draw_commands._length == 0 && dev_draw_state.fill_draw_commands._length == 0) {
		return;
	}

	glUseProgram(dev_draw_state.program.gl_handle);

	gl_program_uniform_set(&dev_draw_state.u_proj_mat, 1, p_projection_matrix);
	gl_program_uniform_set(&dev_draw_state.u_view_mat, 1, p_view_matrix);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	dev_draw_flush_draw_commands(&dev_draw_state.line_draw_commands, delta_time);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	dev_draw_flush_draw_commands(&dev_draw_state.fill_draw_commands, delta_time);
}

int draw_command_cmp(const struct draw_command* p_a, const struct draw_command* p_b) {
	return 
		p_a->remaining_duration > p_b->remaining_duration ?  1 :
		p_a->remaining_duration < p_b->remaining_duration ? -1 :
		0;
}

void dev_draw_init(void) {
	static int b_initialized = 0;

	if (b_initialized) {
		return;
	}

	struct gl_shader gl_vertex_shader;
	struct gl_shader gl_fragment_shader;

	gl_shader_create(&gl_vertex_shader, GL_VERTEX_SHADER);
	gl_shader_compile(&gl_vertex_shader,
		"#version 330 core\n"
		"uniform mat4 u_projection_matrix;"
		"uniform mat4 u_view_matrix;"
		"uniform mat4 u_model_matrix;"
		"layout (location=0) in vec3 a_pos;"
		"void main() {"
			"gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(a_pos, 1.0);"
		"}");

	gl_shader_create(&gl_fragment_shader, GL_FRAGMENT_SHADER);
	gl_shader_compile(&gl_fragment_shader,
		"#version 330 core\n"
		"uniform vec3 u_color;"
		"out vec4 f_color;"
		"void main() {"
			"f_color = vec4(u_color, 1.0);"
		"}");

	gl_program_create(&dev_draw_state.program);
	gl_program_attach_shader(&dev_draw_state.program, &gl_vertex_shader);
	gl_program_attach_shader(&dev_draw_state.program, &gl_fragment_shader);
	gl_program_link(&dev_draw_state.program);

	gl_shader_destroy(&gl_vertex_shader);
	gl_shader_destroy(&gl_fragment_shader);

	gl_program_get_uniform(&dev_draw_state.program, "u_projection_matrix", &dev_draw_state.u_proj_mat);
	gl_program_get_uniform(&dev_draw_state.program, "u_view_matrix", &dev_draw_state.u_view_mat);
	gl_program_get_uniform(&dev_draw_state.program, "u_model_matrix", &dev_draw_state.u_modl_mat);
	gl_program_get_uniform(&dev_draw_state.program, "u_color", &dev_draw_state.u_color);

	darr_init(&dev_draw_state.line_draw_commands, sizeof(struct draw_command));
	darr_init(&dev_draw_state.fill_draw_commands, sizeof(struct draw_command));

	b_initialized = 1;
}

void dev_draw_mesh_internal(const struct gl_mesh* p_gl_mesh, const float* p_transform, u32_r8g8b8a8 line_color, u32_r8g8b8a8 fill_color, float duration) {
	if (line_color == CX_COLOR_NONE && fill_color == CX_COLOR_NONE) {
		return;
	}

	if (line_color != CX_COLOR_NONE) {
		struct draw_command* p_new_command = darr_push(&dev_draw_state.line_draw_commands);
		*p_new_command = (struct draw_command) {
			.p_gl_mesh = p_gl_mesh,
			.color = line_color,
			.remaining_duration = duration
		};
		matrix_copy(p_transform, p_new_command->transform);
	}

	if (fill_color != CX_COLOR_NONE) {
		struct draw_command* p_new_command = darr_push(&dev_draw_state.fill_draw_commands);
		*p_new_command = (struct draw_command) {
			.p_gl_mesh = p_gl_mesh,
			.color = fill_color,
			.remaining_duration = duration
		};
		matrix_copy(p_transform, p_new_command->transform);
	}
}

void dev_draw_flush_draw_commands(struct darr* p_draw_commands, float delta_time) {
	qsort(
		p_draw_commands->_p_buffer, 
		p_draw_commands->_length, 
		p_draw_commands->_element_size, 
		(void*)draw_command_cmp);
	
	size_t new_len = 0;

	for (size_t i = 0; i < p_draw_commands->_length; ++i) {
		struct draw_command* p_command = darr_get(p_draw_commands, i);
		
		p_command->remaining_duration -= delta_time;

		if (p_command->remaining_duration > 0) {
			++new_len;
		}

		struct color_f32 color;
		color_f32_from_u32_r8g8b8a8(&color, p_command->color);
		
		gl_program_uniform_set(&dev_draw_state.u_modl_mat, 1, p_command->transform);
		gl_program_uniform_set(&dev_draw_state.u_color, 1, color.rgba);

		gl_mesh_draw(p_command->p_gl_mesh);
	}

	darr_set_length(p_draw_commands, new_len);
}