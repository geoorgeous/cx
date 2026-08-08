#include <stdlib.h>

#include "cx_alloc.h"
#include "cx_blueprint.h"
#include "cx_stream.h"
#include "cx_stream_serialization.h"
#include "cx_logging.h"
#include "cx_macro.h"

static struct cx_blueprint_node* cx_blueprint_find_node(const struct cx_blueprint* p_blueprint, uint16_t node_id);

void cx_blueprint_destroy(struct cx_blueprint *p_blueprint) {
	for (size_t i = 0; i < p_blueprint->nodes.length; ++i) {
		struct cx_blueprint_node* p_node = cx_array_at(&p_blueprint->nodes, i);
		free(p_node->p_components);
		free(p_node->p_component_data);
	}
	cx_array_free(&p_blueprint->nodes);
	*p_blueprint = (struct cx_blueprint){0};
}

uint16_t cx_blueprint_create_node(struct cx_blueprint* p_blueprint) {
	if (p_blueprint->nodes.element_size == 0) {
		cx_array_init(sizeof(struct cx_blueprint_node), &p_blueprint->nodes);
	}

	struct cx_blueprint_node* p_new_node = cx_array_push(&p_blueprint->nodes, &(struct cx_blueprint_node) {
		.id = p_blueprint->next_node_id++,
		.parent_id = CX_BLUEPRINT_NODE_INVALID_ID
	});

	transform_make_identity(&p_new_node->transform);

	return p_new_node->id;
}

void cx_blueprint_destroy_node(struct cx_blueprint* p_blueprint, uint16_t node_id) {
	// todo: orphan children
	for (size_t i = 0; i < p_blueprint->nodes.length; ++i) {
		struct cx_blueprint_node* p_node = cx_array_at(&p_blueprint->nodes, i);

		if (p_node->id == node_id) {
			free(p_node->p_components);
			cx_array_unordered_remove_at(&p_blueprint->nodes, i);
			break;
		}
	}
}

struct transform* cx_blueprint_node_get_transform(const struct cx_blueprint* p_blueprint, uint16_t node_id) {
	struct cx_blueprint_node* p_node = cx_blueprint_find_node(p_blueprint, node_id);
	return &p_node->transform;
}

uint16_t cx_blueprint_node_get_parent(const struct cx_blueprint* p_blueprint, uint16_t node_id) {
	const struct cx_blueprint_node* p_node = cx_blueprint_find_node(p_blueprint, node_id);
	if (p_node) {
		return p_node->parent_id;
	}
	return CX_BLUEPRINT_NODE_INVALID_ID;
}

void cx_blueprint_node_set_parent(const struct cx_blueprint* p_blueprint, uint16_t node_id, uint16_t parent_node_id) {
	struct cx_blueprint_node* p_node = cx_blueprint_find_node(p_blueprint, node_id);
	if (p_node) {
		p_node->parent_id = parent_node_id;
	}
}

const struct cx_blueprint_node_component* cx_blueprint_node_get_components(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	void** pp_out_component_data,
	size_t* p_out_n) {

	const struct cx_blueprint_node* p_node = cx_blueprint_find_node(p_blueprint, node_id);

	if (p_node) {
		*pp_out_component_data = p_node->p_component_data;
		*p_out_n = p_node->components_count;
		return p_node->p_components;
	}

	*pp_out_component_data = 0;
	*p_out_n = 0;
	return 0;
}

void* cx_blueprint_node_add_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type) {

	struct cx_blueprint_node* p_node = cx_blueprint_find_node(p_blueprint, node_id);

	if (cx_blueprint_node_has_component(p_blueprint, node_id, p_type)) {
		return CX_NULL;
	}

	const struct cx_blueprint_node_component* p_last =
		p_node->components_count ?
		p_node->p_components + p_node->components_count - 1 :
		0;

	const size_t component_data_old_size =
		p_last ?
		p_last->data_off + p_last->p_type->size :
		0;

	const size_t components_new_size = sizeof(*p_node->p_components) * (p_node->components_count + 1);
	const size_t component_data_new_size = component_data_old_size + p_type->size;

	p_node->p_components = realloc(p_node->p_components, components_new_size);
	p_node->p_component_data = realloc(p_node->p_component_data, component_data_new_size);

	struct cx_blueprint_node_component* p_new_component = p_node->p_components + p_node->components_count;
	*p_new_component = (struct cx_blueprint_node_component) {
		.p_type = p_type,
		.data_off = component_data_old_size
	};

	p_node->components_count++;

	return p_node->p_component_data + p_new_component->data_off;
}

void cx_blueprint_node_remove_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type) {

	struct cx_blueprint_node* p_node = cx_blueprint_find_node(p_blueprint, node_id);

	if (p_node) {
		for (size_t i = 0; i < p_node->components_count; ++i) {
			if (p_node->p_components[i].p_type->runtime_id == p_type->runtime_id) {
				// todo: swap-back-remove component info
				// todo: memove component data byte
				break;
			}
		}
	}
}

void* cx_blueprint_node_find_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type) {

	const struct cx_blueprint_node* p_node = cx_blueprint_find_node(p_blueprint, node_id);

	for (size_t i = 0; i < p_node->components_count; ++i) {
		if (p_node->p_components[i].p_type->runtime_id == p_type->runtime_id) {
			return p_node->p_component_data + p_node->p_components[i].data_off;
		}
	}

	return CX_NULL;
}

int cx_blueprint_node_has_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type) {
	
	return cx_blueprint_node_find_component(p_blueprint, node_id, p_type) != 0;
}

struct cx_blueprint_node* cx_blueprint_find_node(const struct cx_blueprint* p_blueprint, uint16_t node_id) {
	for (size_t i = 0; i < p_blueprint->nodes.length; ++i) {
		struct cx_blueprint_node* p_node = cx_array_at(&p_blueprint->nodes, i);		
		if (p_node->id == node_id) {
			return p_node;
		}
	}

	return CX_NULL;
}

int cx_blueprint_serialize(const struct cx_blueprint* p_blueprint, struct cx_stream* p_stream) {
	cx_stream_serialize_uint16(p_stream, (uint16_t)p_blueprint->nodes.length);

	CX_LOG_FMT(INFO, STREAM_WRITE, "Serializing blueprint asset (%u blueprint nodes)...\n", p_blueprint->nodes.length);

	for (uint16_t i = 0; i < p_blueprint->nodes.length; ++i) {
		const struct cx_blueprint_node* p_node = cx_array_at(&p_blueprint->nodes, i);

		cx_stream_serialize_uint16(p_stream, p_node->id);
		cx_stream_serialize_uint16(p_stream, p_node->parent_id);

		cx_stream_serialize_bytes(p_stream,
			sizeof(p_node->transform.position), p_node->transform.position);
		cx_stream_serialize_bytes(p_stream,
			sizeof(p_node->transform.rotation), p_node->transform.rotation);
		cx_stream_serialize_bytes(p_stream,
			sizeof(p_node->transform.scale), p_node->transform.scale);
		cx_stream_serialize_bytes(p_stream,
			sizeof(p_node->transform.world_position), p_node->transform.world_position);
		cx_stream_serialize_bytes(p_stream,
			sizeof(p_node->transform.world_rotation), p_node->transform.world_rotation);
		cx_stream_serialize_bytes(p_stream,
			sizeof(p_node->transform.world_scale), p_node->transform.world_scale);

		const struct cx_blueprint_node_component* p_last =
			p_node->components_count ?
			p_node->p_components + p_node->components_count - 1 :
			0;

		const size_t component_data_size =
			p_last ?
			p_last->data_off + p_last->p_type->size :
			0;

		cx_stream_serialize_uint16(p_stream, p_node->components_count);
		cx_stream_serialize_uint64(p_stream, component_data_size);

		CX_LOG_FMT(INFO, STREAM_WRITE, "  Serializing node (%u component(s), %"CX_PRI_SIZE" bytes)...\n",
			p_node->components_count, component_data_size);

		for (uint16_t j = 0; j < p_node->components_count; ++j) {
			const struct cx_blueprint_node_component* p_node_component = &p_node->p_components[j];

			CX_LOG_FMT(INFO, STREAM_WRITE,
				"    Serializing component (%s, data_off=%"CX_PRI_SIZE", %"CX_PRI_SIZE" bytes)...\n",
				p_node_component->p_type->s_name, p_node_component->data_off, component_data_size);

			cx_component_serialize(
				p_node->p_component_data + p_node_component->data_off, p_node_component->p_type, p_stream);
		}
	}

	return CX_TRUE;
}

int cx_blueprint_deserialize(struct cx_stream* p_stream, struct cx_blueprint* p_out_blueprint) {
	uint16_t num_nodes;
	cx_stream_deserialize_uint16(p_stream, &num_nodes);

	CX_LOG_FMT(INFO, STREAM_READ, "Deserializing blueprint asset (%u blueprint nodes)...\n", num_nodes);

	cx_array_init_capacity(sizeof(struct cx_blueprint_node), num_nodes, &p_out_blueprint->nodes);

	for (uint16_t i = 0; i < num_nodes; ++i) {
		struct cx_blueprint_node* p_node = cx_array_push(&p_out_blueprint->nodes, CX_NULL);

		cx_stream_deserialize_uint16(p_stream, &p_node->id);
		cx_stream_deserialize_uint16(p_stream, &p_node->parent_id);

		p_node->transform.p_local_transform = CX_NULL;

		cx_stream_deserialize_bytes(p_stream,
			sizeof(p_node->transform.position), p_node->transform.position);
		cx_stream_deserialize_bytes(p_stream,
			sizeof(p_node->transform.rotation), p_node->transform.rotation);
		cx_stream_deserialize_bytes(p_stream,
			sizeof(p_node->transform.scale), p_node->transform.scale);
		cx_stream_deserialize_bytes(p_stream,
			sizeof(p_node->transform.world_position), p_node->transform.world_position);
		cx_stream_deserialize_bytes(p_stream,
			sizeof(p_node->transform.world_rotation), p_node->transform.world_rotation);
		cx_stream_deserialize_bytes(p_stream,
			sizeof(p_node->transform.world_scale), p_node->transform.world_scale);

		cx_stream_deserialize_uint16(p_stream, &p_node->components_count);

		size_t component_data_size;
		cx_stream_deserialize_uint64(p_stream, &component_data_size);

		CX_LOG_FMT(INFO, STREAM_READ, "  Deserializing node (%u component(s), %"CX_PRI_SIZE" bytes)...\n",
			p_node->components_count, component_data_size);

		p_node->p_components = CX_MALLOC(sizeof(*p_node->p_components) * p_node->components_count);
		p_node->p_component_data = CX_MALLOC(component_data_size);

		size_t component_data_off = 0;

		for (uint16_t j = 0; j < p_node->components_count; ++j) {
			struct cx_blueprint_node_component* p_node_component = &p_node->p_components[j];
			*p_node_component = (struct cx_blueprint_node_component) {
				.data_off = component_data_off
			};
			
			cx_component_deserialize(
				p_stream, &p_node_component->p_type, p_node->p_component_data + p_node_component->data_off);

			CX_LOG_FMT(INFO, STREAM_WRITE,
				"    Deserializing component (%s, data_off=%"CX_PRI_SIZE", %"CX_PRI_SIZE" bytes)...\n",
				p_node_component->p_type->s_name, component_data_off, component_data_size);

			component_data_off += p_node_component->p_type->size;
		}
	}

	return CX_TRUE;
}

void cx_blueprint_asset_enumerate_dependencies(
	const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr) {

	const struct cx_blueprint* p_blueprint = p_asset;

	for (uint16_t i = 0; i < p_blueprint->nodes.length; ++i) {
		const struct cx_blueprint_node* p_node = cx_array_at(&p_blueprint->nodes, i);

		for (uint16_t j = 0; j < p_node->components_count; ++j) {
			const struct cx_blueprint_node_component* p_node_component = &p_node->p_components[j];

			const void* p_component = p_node->p_component_data + p_node_component->data_off;

			cx_component_enumerate_asset_dependencies(p_node_component->p_type, p_component, f_cb, p_user_ptr);
		}
	}
}
