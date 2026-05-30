#include <stdlib.h>

#include "cx_blueprint.h"

static struct cx_blueprint_node* cx_blueprint_find_node(const struct cx_blueprint* p_blueprint, uint16_t node_id);

void cx_blueprint_free(struct cx_blueprint *p_blueprint) {
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
