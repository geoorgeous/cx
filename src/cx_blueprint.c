#include <stdlib.h>

#include "cx_blueprint.h"
#include "cx_stream_serialization.h"

static struct cx_blueprint_node* cx_blueprint_find_node(const struct cx_blueprint* p_blueprint, uint16_t node_id);

void cx_blueprint_destroy(struct cx_blueprint *p_blueprint) {
	for (size_t i = 0; i < p_blueprint->nodes_count; ++i) {
		free(p_blueprint->p_nodes[i].p_components);
		free(p_blueprint->p_nodes[i].p_component_data);
	}
	free(p_blueprint->p_nodes);
	*p_blueprint = (struct cx_blueprint){0};
}

uint16_t cx_blueprint_create_node(struct cx_blueprint* p_blueprint) {
	if (p_blueprint->nodes_capacity == p_blueprint->nodes_count) {
		p_blueprint->nodes_capacity++;
		p_blueprint->p_nodes = realloc(p_blueprint->p_nodes, sizeof(*p_blueprint->p_nodes) * p_blueprint->nodes_capacity);
	}
	
	struct cx_blueprint_node* p_new_node = p_blueprint->p_nodes + p_blueprint->nodes_count++;

	*p_new_node = (struct cx_blueprint_node) {
		.id = p_blueprint->next_node_id++,
		.parent_id = CX_BLUEPRINT_NODE_INVALID_ID
	};

	transform_make_identity(&p_new_node->transform);

	return p_new_node->id;
}

void cx_blueprint_destroy_node(struct cx_blueprint* p_blueprint, uint16_t node_id) {
	// todo: orphan children
	for (size_t i = 0; i < p_blueprint->nodes_count; ++i) {
		if (p_blueprint->p_nodes[i].id == node_id) {
			free(p_blueprint->p_nodes[i].p_components);
			if (i < p_blueprint->nodes_count - 1) {
				p_blueprint->p_nodes[i] = p_blueprint->p_nodes[p_blueprint->nodes_count - 1];
			}
			p_blueprint->nodes_count--;
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
		return 0;
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

	return 0;
}

int cx_blueprint_node_has_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type) {
	
	return cx_blueprint_node_find_component(p_blueprint, node_id, p_type) != 0;
}

struct cx_blueprint_node* cx_blueprint_find_node(const struct cx_blueprint* p_blueprint, uint16_t node_id) {
	for (size_t i = 0; i < p_blueprint->nodes_count; ++i) {
		if (p_blueprint->p_nodes[i].id == node_id) {
			return p_blueprint->p_nodes + i;
		}
	}

	return 0;
}

int cx_blueprint_serialize(const struct cx_blueprint* p_blueprint, struct cx_stream_writer* p_writer) {
	cx_stream_serialize_uint16(p_writer, p_blueprint->nodes_count);

	for (uint16_t i = 0; i < p_blueprint->nodes_count; ++i) {
		const struct cx_blueprint_node* p_node = &p_blueprint->p_nodes[i];

		cx_stream_serialize_uint16(p_writer, p_node->id);
		cx_stream_serialize_uint16(p_writer, p_node->parent_id);

		cx_stream_serialize_bytes(p_writer,
			sizeof(p_node->transform.position), p_node->transform.position);
		cx_stream_serialize_bytes(p_writer,
			sizeof(p_node->transform.rotation), p_node->transform.rotation);
		cx_stream_serialize_bytes(p_writer,
			sizeof(p_node->transform.scale), p_node->transform.scale);
		cx_stream_serialize_bytes(p_writer,
			sizeof(p_node->transform.world_position), p_node->transform.world_position);
		cx_stream_serialize_bytes(p_writer,
			sizeof(p_node->transform.world_rotation), p_node->transform.world_rotation);
		cx_stream_serialize_bytes(p_writer,
			sizeof(p_node->transform.world_scale), p_node->transform.world_scale);

		cx_stream_serialize_uint16(p_writer, p_node->components_count);

		for (uint16_t j = 0; j < p_node->components_count; ++j) {
			const struct cx_blueprint_node_component* p_node_component = &p_node->p_components[j];
			cx_stream_serialize_uint64(p_writer, p_node_component->data_off);
			cx_component_serialize(
				p_node->p_component_data + p_node_component->data_off, p_node_component->p_type, p_writer);
		}
	}

	return CX_TRUE;
}

int cx_blueprint_deserialize(struct cx_blueprint* p_blueprint, struct cx_stream_reader* p_reader) {
	cx_stream_deserialize_uint16(p_reader, &p_blueprint->nodes_count);

	// alloc nodes

	for (uint16_t i = 0; i < p_blueprint->nodes_count; ++i) {
		struct cx_blueprint_node* p_node = &p_blueprint->p_nodes[i];

		cx_stream_deserialize_uint16(p_reader, &p_node->id);
		cx_stream_deserialize_uint16(p_reader, &p_node->parent_id);

		cx_stream_deserialize_bytes(p_reader,
			sizeof(p_node->transform.position), p_node->transform.position);
		cx_stream_deserialize_bytes(p_reader,
			sizeof(p_node->transform.rotation), p_node->transform.rotation);
		cx_stream_deserialize_bytes(p_reader,
			sizeof(p_node->transform.scale), p_node->transform.scale);
		cx_stream_deserialize_bytes(p_reader,
			sizeof(p_node->transform.world_position), p_node->transform.world_position);
		cx_stream_deserialize_bytes(p_reader,
			sizeof(p_node->transform.world_rotation), p_node->transform.world_rotation);
		cx_stream_deserialize_bytes(p_reader,
			sizeof(p_node->transform.world_scale), p_node->transform.world_scale);

		cx_stream_deserialize_uint16(p_reader, &p_node->components_count);

		// alloc components and component data

		for (uint16_t j = 0; j < p_node->components_count; ++j) {
			struct cx_blueprint_node_component* p_node_component = &p_node->p_components[j];
			cx_stream_deserialize_uint64(p_reader, &p_node_component->data_off);
			cx_component_deserialize(
				p_node->p_component_data + p_node_component->data_off, p_reader, &p_node_component->p_type);
		}
	}

	return CX_TRUE;
}
