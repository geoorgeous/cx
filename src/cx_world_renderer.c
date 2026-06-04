#include "cx_cmp_static_mesh.h"
#include "cx_image.h"
#include "cx_object_id_capturer.h"
#include "cx_render_pass.h"
#include "cx_texture.h"
#include "cx_world.h"
#include "cx_world_renderer.h"
#include "material.h"
#include "matrix.h"
#include "static_mesh.h"

static struct cx_gfx_texture texture_white_1x1;
static float color_white[3] = { 1, 1, 1 };

static void cx_world_renderer_init(void);

void cx_world_renderer_record_forward_pass_commands(
	const struct cx_world* p_world,
	struct cx_render_command_buffer* p_render_command_buffer) {

	cx_world_renderer_init();

	const struct cx_component_pool* p_pool = cx_world_get_component_pool(p_world, &cmp_type_static_mesh);
	const struct cx_cmp_static_mesh* p_static_meshes = (const void*)p_pool->p_dense_components;

	for (size_t i = 0; i < p_pool->count; ++i) {
		struct static_mesh* p_static_mesh = p_static_meshes[i].p_asset_package_record->asset_.p_data_;
		
		const struct transform* p_transform =
			cx_world_entity_get_transform_const(p_world, p_pool->p_dense_entities[i]);

		if (!p_static_mesh->b_loaded_device_meshes) {
			static_mesh_load_device_meshes(p_static_mesh);
		}

		for (size_t j = 0; j < p_static_mesh->num_primitives; ++j) {
			const struct cx_gfx_texture* p_gfx_texture = &texture_white_1x1;
			const float* p_color = color_white;

			if (p_static_mesh->p_materials[j]) {
				const struct material* p_material = p_static_mesh->p_materials[j]->asset_.p_data_;
				if (p_material->p_texture) {
					struct cx_texture* p_texture = p_material->p_texture->asset_.p_data_;
					cx_texture_load_gfx_texture(p_texture, 0);
					p_gfx_texture = &p_texture->gfx_texture_;
				}
				p_color = p_material->color;
			}

			cx_render_command_buffer_push(p_render_command_buffer, &((struct cx_render_command) {
				.p_mesh = &p_static_mesh->p_gfx_meshes[j],
				.p_object_data = p_transform->world_trs_matrix,
				.p_material_data = p_color,
				.p_opaque_resources = { p_gfx_texture },
				.num_opaque_params = 1
			}));
		}
	}
}

void cx_world_renderer_record_picker_pass_commands(
	const struct cx_world* p_world,
	struct cx_render_command_buffer* p_render_command_buffer) {

	static struct {
		float transform[16];
		uint32_t object_id;
	} object_data[1024];

	const struct cx_component_pool* p_pool = cx_world_get_component_pool(p_world, &cmp_type_static_mesh);
	const struct cx_cmp_static_mesh* p_static_meshes = (const void*)p_pool->p_dense_components;

	for (size_t i = 0; i < p_pool->count; ++i) {
		struct static_mesh* p_static_mesh = p_static_meshes[i].p_asset_package_record->asset_.p_data_;

		const struct transform* p_transform =
			cx_world_entity_get_transform_const(p_world, p_pool->p_dense_entities[i]);

		if (!p_static_mesh->b_loaded_device_meshes) {
			static_mesh_load_device_meshes(p_static_mesh);
		}

		for (size_t j = 0; j < p_static_mesh->num_primitives; ++j) {
			matrix_copy(p_transform->world_trs_matrix, object_data[p_render_command_buffer->num].transform);

			object_data[p_render_command_buffer->num].object_id =
				CX_OBJECT_ID_MAKE(CX_OBJECT_ID_CATEGORY_ENTITY, p_pool->p_dense_entities[i]);

			cx_render_command_buffer_push(p_render_command_buffer, &((struct cx_render_command) {
				.p_mesh = p_static_mesh->p_gfx_meshes + j,
				.p_object_data = &object_data[p_render_command_buffer->num],
			}));
		}
	}
}

static void cx_world_renderer_init(void) {
	static int b_done = 0;

	if (b_done) {
		return;
	}

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
