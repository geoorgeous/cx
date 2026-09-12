#include "cx_asset_cache.h"
#include "cx_cmp_static_mesh.h"
#include "cx_gfx_mesh.h"
#include "cx_material.h"
#include "cx_object_id_capturer.h"
#include "cx_render_draw_command.h"
#include "cx_shader.h"
#include "cx_world.h"
#include "cx_world_renderer.h"
#include "matrix.h"
#include "static_mesh.h"

static struct cx_gfx_shader_program_input_block g_draw_shader_program_input_set_blocks[1024];

static struct cx_asset_ref g_object_id_shader_asset_ref;

void cx_world_renderer_record_draw_commands_lit(
	const struct cx_world* p_world,
	struct cx_render_draw_command_buffer* p_render_draw_command_buffer) {

	const struct cx_component_pool* p_pool = cx_world_get_component_pool(p_world, &cmp_type_static_mesh);
	struct cx_cmp_static_mesh* p_static_meshes = (void*)p_pool->p_dense_components;

	for (size_t i = 0; i < p_pool->count; ++i) {
		struct static_mesh* p_static_mesh = cx_asset_cache_acquire(&p_static_meshes[i].asset_ref);

		if (!p_static_mesh->b_loaded_device_meshes) {
			static_mesh_load_device_meshes(p_static_mesh);
		}

		const struct transform* p_transform =
			cx_world_entity_get_transform_const(p_world, p_pool->p_dense_entities[i]);

		g_draw_shader_program_input_set_blocks[i].s_name = "blk_object";
		g_draw_shader_program_input_set_blocks[i].p_data = p_transform->world_trs_matrix;

		for (size_t j = 0; j < p_static_mesh->num_primitives; ++j) {
			struct cx_asset_ref* p_material_asset_ref = &p_static_mesh->p_primitives_material_asset_refs[j];
			struct cx_material* p_material = cx_asset_cache_acquire(p_material_asset_ref);
			struct cx_shader* p_pipeline_shader = cx_asset_cache_acquire(&p_material->pipeline_shader_asset_ref);

			struct cx_render_draw_command draw_command = {
				.pipeline = {
					.p_shader = p_pipeline_shader,
					.state = p_material->pipeline_state
				},
				.material_input_set = p_material->shader_program_input_set,
				.draw_input_set = {
					.p_blocks = &g_draw_shader_program_input_set_blocks[i],
					.num_blocks = 1
				},
				.p_mesh = &p_static_mesh->p_gfx_meshes[j]
			};

			cx_render_draw_command_buffer_push(p_render_draw_command_buffer, &draw_command);
		}
	}
}

void cx_world_renderer_record_draw_commands_object_id(
	const struct cx_world* p_world,
	struct cx_render_draw_command_buffer* p_render_draw_command_buffer) {

	static struct {
		float transform[16];
		uint32_t object_id;
	} object_data[1024];

	if (!cx_asset_ref_is_set(&g_object_id_shader_asset_ref)) {
		cx_asset_cache_find_by_name(CX_ASSET_TYPE_SHADER, "shader_object_id", &g_object_id_shader_asset_ref);
	}

	struct cx_shader* p_shader = cx_asset_cache_acquire(&g_object_id_shader_asset_ref);

	const struct cx_component_pool* p_pool = cx_world_get_component_pool(p_world, &cmp_type_static_mesh);
	struct cx_cmp_static_mesh* p_static_meshes = (void*)p_pool->p_dense_components;

	for (size_t i = 0; i < p_pool->count; ++i) {
		struct static_mesh* p_static_mesh = cx_asset_cache_acquire(&p_static_meshes[i].asset_ref);

		if (!p_static_mesh->b_loaded_device_meshes) {
			static_mesh_load_device_meshes(p_static_mesh);
		}

		const struct transform* p_transform =
			cx_world_entity_get_transform_const(p_world, p_pool->p_dense_entities[i]);

		for (size_t j = 0; j < p_static_mesh->num_primitives; ++j) {
			matrix_copy(p_transform->world_trs_matrix, object_data[p_render_draw_command_buffer->len].transform);

			object_data[p_render_draw_command_buffer->len].object_id =
				CX_OBJECT_ID_MAKE(CX_OBJECT_ID_CATEGORY_ENTITY, p_pool->p_dense_entities[i]);

			g_draw_shader_program_input_set_blocks[i].s_name = "blk_object";
			g_draw_shader_program_input_set_blocks[i].p_data = &object_data[i];

			struct cx_render_draw_command draw_command = {
				.pipeline = {
					.p_shader = p_shader,
					.state = {
						.flags =
							CX_RENDER_PIPELINE_FLAG_depth_test_enabled |
							CX_RENDER_PIPELINE_FLAG_depth_writes_enabled,
						.depth_test_func = CX_DEPTH_TEST_FUNC_less,
						.cull_mode = CX_CULL_MODE_back
					}
				},
				.draw_input_set = {
					.p_blocks = &g_draw_shader_program_input_set_blocks[i],
					.num_blocks = 1
				},
				.p_mesh = &p_static_mesh->p_gfx_meshes[j]
			};

			cx_render_draw_command_buffer_push(p_render_draw_command_buffer, &draw_command);
		}
	}
}
