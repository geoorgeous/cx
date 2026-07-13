#ifndef CX_BLUEPRINT_H
#define CX_BLUEPRINT_H

#include "cx_component.h"
#include "transform.h"

#define CX_ASSET_TYPE_BLUEPRINT 7

#define CX_BLUEPRINT_NODE_INVALID_ID UINT16_MAX

struct cx_blueprint_node_component {
	const struct cx_component_type* p_type;
	size_t data_off;
};

struct cx_blueprint_node {
	uint16_t id;
	uint16_t parent_id;
	struct transform transform;
	struct cx_blueprint_node_component* p_components;
	uint16_t components_count;
	uint8_t* p_component_data;
};

struct cx_blueprint {
	struct cx_blueprint_node* p_nodes;
	uint16_t nodes_count;
	uint16_t nodes_capacity;
	uint16_t next_node_id;
};

void cx_blueprint_destroy(struct cx_blueprint* p_blueprint);

uint16_t cx_blueprint_create_node(struct cx_blueprint* p_blueprint);

void cx_blueprint_destroy_node(struct cx_blueprint* p_blueprint, uint16_t node_id);

struct transform* cx_blueprint_node_get_transform(const struct cx_blueprint* p_blueprint, uint16_t node_id);

uint16_t cx_blueprint_node_get_parent(const struct cx_blueprint* p_blueprint, uint16_t node_id);

void cx_blueprint_node_set_parent(const struct cx_blueprint* p_blueprint, uint16_t node_id, uint16_t parent_node_id);

const struct cx_blueprint_node_component* cx_blueprint_node_get_components(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	void** pp_out_component_data,
	size_t* p_out_n); 

void* cx_blueprint_node_add_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type);

void cx_blueprint_node_remove_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type);

void* cx_blueprint_node_find_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type);

int cx_blueprint_node_has_component(
	const struct cx_blueprint* p_blueprint,
	uint16_t node_id,
	const struct cx_component_type* p_type);

struct cx_stream_writer;
struct cx_stream_reader;

int cx_blueprint_serialize(const struct cx_blueprint* p_blueprint, struct cx_stream_writer* p_writer);
int cx_blueprint_deserialize(struct cx_blueprint* p_blueprint, struct cx_stream_reader* p_reader);

static inline void cx_blueprint_asset_destroy(void* p) {
	cx_blueprint_destroy(p);
}

static inline int cx_blueprint_asset_serialize(struct cx_stream_writer* p_writer, const void* p) {
	return cx_blueprint_serialize(p, p_writer);
}

static inline int cx_blueprint_asset_deserialize(struct cx_stream_reader* p_reader, void* p) {
	return cx_blueprint_deserialize(p, p_reader);
}

#endif
