#include "cx_cmp_static_mesh.h"
#include "cx_dbg.h"
#include "cx_gfx_program.h"
#include "cx_image.h"
#include "cx_io.h"
#include "cx_render_pass.h"
#include "cx_texture.h"
#include "cx_world.h"
#include "cx_world_renderer.h"
#include "material.h"
#include "matrix.h"
#include "static_mesh.h"

static struct cx_gfx_program program;

static struct cx_gfx_program_param_block program_pblock_camera;
static struct cx_gfx_program_param_block program_pblock_object;
static struct cx_gfx_program_param_block program_pblock_material;
static struct cx_gfx_program_opaque_param program_opaque_texture_albedo;

static struct cx_gfx_program_param_buffer program_pbuffer_camera;
static struct cx_gfx_program_param_buffer program_pbuffer_object;
static struct cx_gfx_program_param_buffer program_pbuffer_material;

static struct cx_render_pass render_pass;

static struct cx_gfx_texture texture_white_1x1;

static void cx_world_renderer_init(void);

void cx_world_renderer_draw(struct cx_world* p_world, const float* p_projection_matrix, const float* p_view_matrix) {
	cx_world_renderer_init();

	const float material_color[] = { 1, 1, 1 };

	struct cx_gfx_program_opaque_param_binding opaque_bindings[1024];
	struct cx_render_pass_command commands[1024];
	size_t num_commands = 0;

	const struct cx_component_pool* p_pool = cx_world_get_component_pool(p_world, &cmp_type_static_mesh);
	const struct cx_cmp_static_mesh* p_static_meshes = (const void*)p_pool->p_dense_components;

	for (size_t i = 0; i < p_pool->count; ++i) {
		struct static_mesh* p_static_mesh = p_static_meshes[i].p_asset_package_record->asset_.p_data_;
		struct transform* p_transform = cx_world_entity_get_transform(p_world, p_pool->p_dense_entities[i]);

		transform_compute_world_trs_matrix(p_transform);

		if (!p_static_mesh->b_loaded_device_meshes) {
			static_mesh_load_device_meshes(p_static_mesh);
		}

		for (size_t j = 0; j < p_static_mesh->num_primitives; ++j) {
			const struct cx_gfx_texture* p_gfx_texture = &texture_white_1x1;

			if (p_static_mesh->p_materials[j]) {
				const struct material* p_material = p_static_mesh->p_materials[j]->asset_.p_data_;
				if (p_material->p_texture) {
					struct cx_texture* p_texture = p_material->p_texture->asset_.p_data_;
					cx_texture_load_gfx_texture(p_texture, 0);
					p_gfx_texture = &p_texture->gfx_texture_;
				}
			}

			opaque_bindings[num_commands] = (struct cx_gfx_program_opaque_param_binding) {
				.p_param = &program_opaque_texture_albedo,
				.p_resource = p_gfx_texture
			};
			
			commands[num_commands] = (struct cx_render_pass_command) {
				.p_mesh = p_static_mesh->p_gfx_meshes + j,
				.p_object_data = p_transform->world_trs_matrix,
				.p_material_data = material_color,
				.p_opaque_params = opaque_bindings + num_commands,
				.num_opaque_params = 1
			};

			num_commands++;
		}
	}

	float camera[32];
	matrix_copy(p_projection_matrix, &camera[0]);
	matrix_copy(p_view_matrix, &camera[16]);

	struct cx_render_pass_data render_pass_data = {
		.p_data = camera
	};

	cx_render_pass_execute(&render_pass, &render_pass_data, commands, num_commands);
}

static void cx_world_renderer_init(void) {
	static int b_done = 0;

	if (b_done) {
		return;
	}

	void* p_vsource;
	void* p_fsource;
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/lit.vert", (void**)&p_vsource, 0) == CX_ERROR_none);
	CX_ASSERT(cx_io_file_read_all("res/builtin/shd/lit.frag", (void**)&p_fsource, 0) == CX_ERROR_none);

	struct cx_gfx_program_source program_source = {
		.s_vertex_stage_source = p_vsource,
		.s_fragment_stage_source = p_fsource
	};

	CX_ASSERT(cx_gfx_program_create(&program) == CX_ERROR_none);
	CX_ASSERT(cx_gfx_program_build(&program, &program_source) == CX_ERROR_none);

	cx_io_file_free(p_vsource);
	cx_io_file_free(p_fsource);

	cx_gfx_program_refl_param_block(&program, "blk_camera", &program_pblock_camera);
	cx_gfx_program_refl_param_block(&program, "blk_object", &program_pblock_object);
	cx_gfx_program_refl_param_block(&program, "blk_material_properties", &program_pblock_material);
	cx_gfx_program_refl_opaque_param(&program, "u_texture_albedo", &program_opaque_texture_albedo);

	cx_gfx_program_param_buffer_create(&program_pbuffer_camera, program_pblock_camera.size_);
	cx_gfx_program_param_buffer_create(&program_pbuffer_object, program_pblock_object.size_);
	cx_gfx_program_param_buffer_create(&program_pbuffer_material, program_pblock_material.size_);

	render_pass = (struct cx_render_pass) {
		.p_program = &program,
		.p_pass_block = &program_pblock_camera,
		.p_pass_buffer = &program_pbuffer_camera,
		.p_object_block = &program_pblock_object,
		.p_object_buffer = &program_pbuffer_object,
		.p_material_block = &program_pblock_material,
		.p_material_buffer = &program_pbuffer_material,
	};

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

	cx_gfx_texture_create(&texture_white_1x1, white_image.width, white_image.height, CX_PIXEL_FORMAT_rgb);
	cx_gfx_texture_set_data(&texture_white_1x1, white_image.p_pixel_data, &white_image.pixel_data_format);

	b_done = 1;
}
