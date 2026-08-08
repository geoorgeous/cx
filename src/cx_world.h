#ifndef CX_WORLD_H
#define CX_WORLD_H

#include <stddef.h>
#include <stdint.h>

#include "cx_component.h"
#include "transform.h"

#define CX_LOG_CAT_WORLD "world"

#define CX_WORLD_MAX_ENTITIES 1024

#define CX_ENTITY_ID_INVALID UINT16_MAX

struct cx_entity {
	struct transform transform;
	int b_alive;
};

struct cx_component_pool {
	const struct cx_component_type* p_type;

	uint16_t  sparse[CX_WORLD_MAX_ENTITIES];
	uint16_t* p_dense_entities;
	uint8_t*  p_dense_components;

	uint16_t count;
	uint16_t capacity;
};

struct cx_component_pool_def {
	const struct cx_component_type* p_type;
	size_t capacity;
};

struct cx_world {
	struct cx_entity entities[CX_WORLD_MAX_ENTITIES];
	uint16_t         free_entities[CX_WORLD_MAX_ENTITIES];
	uint16_t         num_free_entities;

	struct cx_component_pool* p_component_pools;
	uint16_t                  num_component_pools;
	uint16_t                  component_type_pool_ids[CX_COMPONENT_MAX_TYPES];

	void* p_buf;
};

void cx_world_init(struct cx_world* p_world, const struct cx_component_pool_def* p_pool_defs, uint16_t num_pool_defs);

void cx_world_free(struct cx_world* p_world);

uint16_t cx_world_entity_create(struct cx_world* p_world);

void cx_world_entity_destroy(struct cx_world* p_world, uint16_t entity_id);

int cx_world_entity_is_alive(const struct cx_world* p_world, uint16_t entity_id);

struct transform* cx_world_entity_get_transform(struct cx_world* p_world, uint16_t entity_id);

const struct transform* cx_world_entity_get_transform_const(const struct cx_world* p_world, uint16_t entity_id);

const struct cx_component_pool* cx_world_get_component_pool(
	const struct cx_world* p_world,
	const struct cx_component_type* p_type);

void* cx_world_component_add(
	struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type);

void cx_world_component_remove(
	struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type);

void* cx_world_component_find(
	const struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type);

int cx_world_component_has(
	const struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type);

struct cx_blueprint;

uint16_t cx_world_instantiate_blueprint(struct cx_world* p_world, const struct cx_blueprint* p_blueprint);

void cx_world_compute_transforms(struct cx_world* p_world);

#endif
