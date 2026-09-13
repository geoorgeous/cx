#include <stdlib.h>
#include <string.h>

#include "cx_alloc.h"
#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_logging.h"
#include "cx_macro.h"
#include "cx_material.h"
#include "cx_stream_serialization.h"
#include "cx_texture.h"

static void cx_material_init_shader_program_input_set(struct cx_material* p_material);

static inline size_t sizeof_cx_gfx_shader_program_value_type(enum cx_gfx_shader_program_value_type type) {
	return (size_t[]) {
		 4,  4,  4,  4,
		 8,  8,  8,
		12, 12, 12,
		16, 16, 16,
		16, 36, 64,
		24, 32,
		24, 48,
		32, 48
	}[type];
}

void cx_material_create(
	const struct cx_asset_ref* p_pipeline_shader_asset_ref,
	const struct cx_render_pipeline_state* p_pipeline_state,
	const struct cx_material_property_info* p_property_info,
	struct cx_material* p_out) {

	*p_out = (struct cx_material){0};

	p_out->pipeline_shader_asset_ref.asset_id = p_pipeline_shader_asset_ref->asset_id;
	p_out->pipeline_state = *p_pipeline_state;

	p_out->p_parameters = CX_MALLOC(sizeof(*p_out->p_parameters) * p_property_info->num_parameters);
	p_out->num_parameters = p_property_info->num_parameters;

	p_out->property_value_buffer_size = 0;

	for (uint16_t i = 0; i < p_property_info->num_parameters; ++i) {
		strcpy(p_out->p_parameters[i].value_view.name, p_property_info->p_parameters[i].s_name);

		p_out->p_parameters[i].value_view.value_buffer_offset = p_out->property_value_buffer_size;

		const size_t value_size = sizeof_cx_gfx_shader_program_value_type(p_property_info->p_parameters[i].type);
		p_out->p_parameters[i].value_view.value_size = value_size;
		p_out->property_value_buffer_size += value_size;

		p_out->p_parameters[i].type = p_property_info->p_parameters[i].type;
	}

	p_out->p_textures = CX_MALLOC(sizeof(*p_out->p_textures) * p_property_info->num_textures);
	p_out->num_textures = p_property_info->num_textures;

	for (uint16_t i = 0; i < p_property_info->num_textures; ++i) {
		strcpy(p_out->p_textures[i].value_view.name, p_property_info->p_textures[i].s_name);

		p_out->p_textures[i].value_view.value_buffer_offset = p_out->property_value_buffer_size;

		const size_t value_size = sizeof(struct cx_asset_ref);
		p_out->p_textures[i].value_view.value_size = value_size;
		p_out->property_value_buffer_size += value_size;
	}
	
	p_out->p_blocks = CX_MALLOC(sizeof(*p_out->p_blocks) * p_property_info->num_blocks);
	p_out->num_blocks = p_property_info->num_blocks;

	for (uint16_t i = 0; i < p_property_info->num_blocks; ++i) {
		strcpy(p_out->p_blocks[i].value_view.name, p_property_info->p_blocks[i].s_name);

		p_out->p_blocks[i].value_view.value_buffer_offset = p_out->property_value_buffer_size;

		p_out->p_blocks[i].p_members =
			CX_MALLOC(sizeof(*p_out->p_blocks[i].p_members) * p_property_info->p_blocks[i].num_members);
		p_out->p_blocks[i].num_members = p_property_info->p_blocks[i].num_members;

		size_t block_size = 0;

		for (uint16_t j = 0; j < p_property_info->p_blocks[i].num_members; ++j) {
			const struct cx_material_block_member_info* p_member_info = &p_property_info->p_blocks[i].p_members[j];
			struct cx_material_block_member* p_member = &p_out->p_blocks[i].p_members[j];

			strcpy(p_member->name, p_member_info->s_name);
			
			p_member->type = p_member_info->type;

			const size_t member_type_size = sizeof_cx_gfx_shader_program_value_type(p_member_info->type);

			p_member->array_len = p_member_info->array_len ? p_member_info->array_len : 1;
			p_member->array_stride = p_member_info->array_stride ? p_member_info->array_stride : member_type_size;
			p_member->matrix_stride = p_member_info->matrix_stride;
			
			p_member->block_offset = block_size;
			p_member->size = p_member->array_stride * p_member->array_len;
			
			block_size += p_member->size;
		}

		p_out->p_blocks[i].value_view.value_size = block_size;
		p_out->property_value_buffer_size += block_size;
	}

	p_out->p_property_value_buffer = CX_CALLOC(p_out->property_value_buffer_size);

	cx_material_init_shader_program_input_set(p_out);
}

void cx_material_create_override(const struct cx_asset_ref* p_parent_asset_ref, struct cx_material* p_out) {
	*p_out = (struct cx_material){0};

	p_out->b_is_override = CX_TRUE;
	p_out->parent_asset_ref.asset_id = p_parent_asset_ref->asset_id;
	p_out->p_overrides = 0; // todo

	const struct cx_material* p_parent_material = cx_asset_cache_acquire(&p_out->parent_asset_ref);

	p_out->pipeline_shader_asset_ref.asset_id = p_parent_material->pipeline_shader_asset_ref.asset_id;
	p_out->pipeline_state = p_parent_material->pipeline_state;

	p_out->p_parameters = p_parent_material->p_parameters;
	p_out->num_parameters = p_parent_material->num_parameters;
	p_out->p_textures = p_parent_material->p_textures;
	p_out->num_textures = p_parent_material->num_textures;
	p_out->p_blocks = p_parent_material->p_blocks;
	p_out->num_blocks = p_parent_material->num_blocks;

	p_out->property_value_buffer_size = p_parent_material->property_value_buffer_size;
	p_out->p_property_value_buffer = CX_MALLOC(p_parent_material->property_value_buffer_size);
	
	memcpy(
		p_out->p_property_value_buffer,
		p_parent_material->p_property_value_buffer,
		p_parent_material->property_value_buffer_size);

	cx_material_init_shader_program_input_set(p_out);
}

void cx_material_free(struct cx_material* p_material) {
	if (!p_material->b_is_override) {
		CX_FREE(p_material->p_parameters);
		
		CX_FREE(p_material->p_textures);
		
		for (uint16_t i = 0; i < p_material->num_blocks; ++i) {
			CX_FREE(p_material->p_blocks[i].p_members);
		}
		CX_FREE(p_material->p_blocks);
	}

	CX_FREE(p_material->p_property_value_buffer);

	CX_FREE(p_material->p_shader_program_input_parameters);
	CX_FREE(p_material->p_shader_program_input_textures);
	CX_FREE(p_material->p_shader_program_input_blocks);

	cx_asset_cache_release(&p_material->pipeline_shader_asset_ref);

	if (p_material->b_is_override) {
		cx_asset_cache_release(&p_material->parent_asset_ref);
	}

	*p_material = (struct cx_material){0};
}

int cx_material_serialize(const struct cx_material* p_material, struct cx_stream* p_stream) {
	cx_stream_serialize_uint8(p_stream, !!p_material->b_is_override);

	cx_asset_ref_serialize(&p_material->pipeline_shader_asset_ref, p_stream);
	
	cx_stream_serialize_bytes(p_stream, sizeof(p_material->pipeline_state), &p_material->pipeline_state);

	if (p_material->b_is_override) {
		cx_asset_ref_serialize(&p_material->parent_asset_ref, p_stream);
	} else {
		cx_stream_serialize_uint16(p_stream, p_material->num_parameters);

		for (uint16_t i = 0; i < p_material->num_parameters; ++i) {
			cx_stream_deserialize_bytes(p_stream, sizeof(p_material->p_parameters[i]), &p_material->p_parameters[i]);
		}

		cx_stream_serialize_uint16(p_stream, p_material->num_textures);

		for (uint16_t i = 0; i < p_material->num_textures; ++i) {
			cx_stream_deserialize_bytes(p_stream, sizeof(p_material->p_textures[i]), &p_material->p_textures[i]);
		}

		cx_stream_serialize_uint16(p_stream, p_material->num_blocks);

		for (uint16_t i = 0; i < p_material->num_blocks; ++i) {
			cx_stream_deserialize_bytes(
				p_stream, sizeof(p_material->p_blocks[i].value_view), &p_material->p_blocks[i].value_view);

			cx_stream_serialize_uint16(p_stream, p_material->p_blocks[i].num_members);

			for (uint16_t j = 0; j < p_material->p_blocks[i].num_members; ++j) {
				cx_stream_serialize_bytes(
					p_stream, sizeof(*p_material->p_blocks[i].p_members), &p_material->p_blocks[i].p_members[j]);
			}
		}
	}

	cx_stream_serialize_uint64(p_stream, p_material->property_value_buffer_size);
	cx_stream_serialize_bytes(p_stream, p_material->property_value_buffer_size, p_material->p_property_value_buffer);

	return CX_TRUE;
}

int cx_material_deserialize(struct cx_stream* p_stream, struct cx_material* p_material) {
	uint8_t temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_material->b_is_override = temp;

	cx_asset_ref_deserialize(p_stream, &p_material->pipeline_shader_asset_ref);

	cx_stream_deserialize_bytes(p_stream, sizeof(p_material->pipeline_state), &p_material->pipeline_state);

	if (p_material->b_is_override) {
		cx_asset_ref_deserialize(p_stream, &p_material->parent_asset_ref);

		cx_material_create_override(&p_material->parent_asset_ref, p_material);
	} else {
		cx_stream_deserialize_uint16(p_stream, &p_material->num_parameters);

		p_material->p_parameters = CX_MALLOC(sizeof(*p_material->p_parameters) * p_material->num_parameters);

		for (uint16_t i = 0; i < p_material->num_parameters; ++i) {
			cx_stream_deserialize_bytes(p_stream, sizeof(p_material->p_parameters[i]), &p_material->p_parameters[i]);
		}

		cx_stream_deserialize_uint16(p_stream, &p_material->num_textures);

		p_material->p_textures = CX_MALLOC(sizeof(*p_material->p_textures) * p_material->num_textures);

		for (uint16_t i = 0; i < p_material->num_textures; ++i) {
			cx_stream_deserialize_bytes(p_stream, sizeof(p_material->p_textures[i]), &p_material->p_textures[i]);
		}

		cx_stream_deserialize_uint16(p_stream, &p_material->num_blocks);

		p_material->p_blocks = CX_MALLOC(sizeof(*p_material->p_blocks) * p_material->num_blocks);

		for (uint16_t i = 0; i < p_material->num_blocks; ++i) {
			cx_stream_deserialize_bytes(
				p_stream, sizeof(p_material->p_blocks[i].value_view), &p_material->p_blocks[i].value_view);

			cx_stream_deserialize_uint16(p_stream, &p_material->p_blocks[i].num_members);

			p_material->p_blocks[i].p_members =
				CX_MALLOC(sizeof(*p_material->p_blocks[i].p_members) * p_material->p_blocks[i].num_members);

			for (uint16_t j = 0; j < p_material->p_blocks[i].num_members; ++j) {
				cx_stream_deserialize_bytes(
					p_stream, sizeof(*p_material->p_blocks[i].p_members), &p_material->p_blocks[i].p_members[j]);
			}
		}

		cx_stream_deserialize_uint64(p_stream, &p_material->property_value_buffer_size);

		p_material->p_property_value_buffer = CX_CALLOC(p_material->property_value_buffer_size);

		cx_material_init_shader_program_input_set(p_material);
	}

	cx_stream_deserialize_bytes(p_stream, p_material->property_value_buffer_size, p_material->p_property_value_buffer);

	return CX_TRUE;
}

void cx_material_set_parameter(struct cx_material* p_material, const char* s_name, const void* p_value) {
	for (uint16_t i = 0; i < p_material->num_parameters; ++i) {
		const struct cx_material_property_value_view* p_view = &p_material->p_parameters[i].value_view;
		if (strcmp(p_view->name, s_name) == 0) {
			void* p_dst = (uint8_t*)p_material->p_property_value_buffer + p_view->value_buffer_offset;
			memcpy(p_dst, p_value, p_view->value_size);
			return;
		}
	}

	CX_LOG_FMT(WARNING, MATERIAL, "Failed to set material parameter '%s': not found\n", s_name);
}

void cx_material_set_texture(
	struct cx_material* p_material, const char* s_name, const struct cx_asset_ref* p_texture_asset_ref) {

	for (uint16_t i = 0; i < p_material->num_textures; ++i) {
		const struct cx_material_property_value_view* p_view = &p_material->p_textures[i].value_view;
		if (strcmp(p_view->name, s_name) != 0) {
			continue;
		}

		struct cx_asset_ref* p_value =
			(void*)((uint8_t*)p_material->p_property_value_buffer + p_view->value_buffer_offset);

		cx_asset_cache_release(p_value);
		
		*p_value = (struct cx_asset_ref) { .asset_id = p_texture_asset_ref->asset_id };

		struct cx_texture* p_texture = cx_asset_cache_acquire(p_value);

		cx_texture_load_gfx_texture(p_texture, CX_FALSE);

		p_material->p_shader_program_input_textures[i].p_texture = &p_texture->gfx_texture_;

		CX_LOG_FMT(TRACE, MATERIAL, "Material texture '%s' set: asset_id=%X\n", s_name, p_value->asset_id);

		return;
	}

	CX_LOG_FMT(WARNING, MATERIAL, "Failed to set material texture '%s': not found\n", s_name);
}

void cx_material_set_block(struct cx_material* p_material, const char* s_name, const void* p_value) {
	for (uint16_t i = 0; i < p_material->num_blocks; ++i) {
		const struct cx_material_property_value_view* p_view = &p_material->p_blocks[i].value_view;
		if (strcmp(p_view->name, s_name) == 0) {
			void* p_dst = (uint8_t*)p_material->p_property_value_buffer + p_view->value_buffer_offset;
			memcpy(p_dst, p_value, p_view->value_size);
			return;
		}
	}

	CX_LOG_FMT(WARNING, MATERIAL, "Failed to set material block '%s': not found\n", s_name);
}

void cx_material_set_block_member(
	struct cx_material* p_material, const char* s_block_name, const char* s_member_name, const void* p_value) {

	for (uint16_t i = 0; i < p_material->num_blocks; ++i) {
		const struct cx_material_property_value_view* p_view = &p_material->p_blocks[i].value_view;
		if (strcmp(p_view->name, s_block_name) != 0) {
			continue;
		}

		for (uint16_t j = 0; j < p_material->p_blocks[i].num_members; ++j) {
			const struct cx_material_block_member* p_member = &p_material->p_blocks[i].p_members[j];

			if (strcmp(p_member->name, s_member_name) != 0) {
				continue;
			}

			void* p_dst =
				(uint8_t*)p_material->p_property_value_buffer + p_view->value_buffer_offset + p_member->block_offset;

			memcpy(p_dst, p_value, p_member->size);

			return;
		}

		CX_LOG_FMT(WARNING, MATERIAL, "Failed to set material block member '%s': no member found in block '%s'\n",
			s_member_name, s_block_name);

		return;
	}

	CX_LOG_FMT(WARNING, MATERIAL, "Failed to set material block member '%s': block '%s' not found\n",
		s_member_name, s_block_name);
}

void cx_material_asset_enumerate_dependencies(
	const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr) {

	const struct cx_material* p_material = p_asset;

	for (uint16_t i = 0; i < p_material->num_textures; ++i) {
		const struct cx_asset_ref* p_texture_asset_ref = (const void*)((const uint8_t*)
			p_material->p_property_value_buffer +
			p_material->p_textures[i].value_view.value_buffer_offset);

		if (cx_asset_ref_is_set(p_texture_asset_ref)) {
			f_cb(p_texture_asset_ref->asset_id, p_user_ptr);
		}
	}
}

void cx_material_init_shader_program_input_set(struct cx_material* p_material) {
	p_material->p_shader_program_input_parameters =
		CX_MALLOC(sizeof(*p_material->p_shader_program_input_parameters) * p_material->num_parameters);

	p_material->p_shader_program_input_textures =
		CX_MALLOC(sizeof(*p_material->p_shader_program_input_textures) * p_material->num_textures);

	p_material->p_shader_program_input_blocks =
		CX_MALLOC(sizeof(*p_material->p_shader_program_input_blocks) * p_material->num_blocks);

	for (uint16_t i = 0; i < p_material->num_parameters; ++i) {
		p_material->p_shader_program_input_parameters[i].s_name = p_material->p_parameters[i].value_view.name;
		p_material->p_shader_program_input_parameters[i].type = p_material->p_parameters[i].type;
		p_material->p_shader_program_input_parameters[i].p_value = 
			(uint8_t*)p_material->p_property_value_buffer + p_material->p_parameters[i].value_view.value_buffer_offset;
	}

	for (uint16_t i = 0; i < p_material->num_textures; ++i) {
		p_material->p_shader_program_input_textures[i].s_name = p_material->p_textures[i].value_view.name;
		p_material->p_shader_program_input_textures[i].p_texture = CX_NULL;
	}

	for (uint16_t i = 0; i < p_material->num_blocks; ++i) {
		p_material->p_shader_program_input_blocks[i].s_name = p_material->p_blocks[i].value_view.name;
		p_material->p_shader_program_input_blocks[i].size = p_material->p_blocks[i].value_view.value_size;
		p_material->p_shader_program_input_blocks[i].p_data = 
			(uint8_t*)p_material->p_property_value_buffer + p_material->p_blocks[i].value_view.value_buffer_offset;
	}

	p_material->shader_program_input_set.p_parameters = p_material->p_shader_program_input_parameters;
	p_material->shader_program_input_set.num_parameters = p_material->num_parameters;
	p_material->shader_program_input_set.p_textures = p_material->p_shader_program_input_textures;
	p_material->shader_program_input_set.num_textures = p_material->num_textures;
	p_material->shader_program_input_set.p_blocks = p_material->p_shader_program_input_blocks;
	p_material->shader_program_input_set.num_blocks = p_material->num_blocks;
}
