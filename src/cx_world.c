#include <stdlib.h>
#include <string.h>

#include "cx_alloc.h"
#include "cx_blueprint.h"
#include "cx_dbg.h"
#include "cx_logging.h"
#include "cx_macro.h"
#include "cx_world.h"

void cx_world_init(struct cx_world* p_world, const struct cx_component_pool_def* p_pool_defs, uint16_t num_pool_defs) {
	const size_t component_pool_size = sizeof(struct cx_component_pool);
	const size_t component_pools_array_size = component_pool_size * num_pool_defs;
	const size_t entity_id_size = sizeof(*((struct cx_component_pool*)0)->p_dense_entities);

	size_t buf_size = component_pool_size * num_pool_defs;

	CX_LOG_FMT(INFO, WORLD, "Initialising world with %"CX_PRI_SIZE" component pools...\n", num_pool_defs);

	for (uint16_t i = 0; i < CX_COMPONENT_MAX_TYPES; ++i) {
		p_world->component_type_pool_ids[i] = CX_COMPONENT_MAX_TYPES;
	}

	for (uint16_t i = 0; i < num_pool_defs; ++i) {
		const size_t dense_arrays_size = (entity_id_size + p_pool_defs[i].p_type->size) * p_pool_defs[i].capacity;
		buf_size = CX_ALIGN_DEFAULT(buf_size);
		buf_size += dense_arrays_size;
		p_world->component_type_pool_ids[p_pool_defs[i].p_type->runtime_id] = i;

		CX_LOG_FMT(INFO, WORLD,
			" [%"CX_PRI_SIZE"] type='%s', runtime_id=%u, size=%"CX_PRI_SIZE", capacity=%"CX_PRI_SIZE"\n",
			i,
			p_pool_defs[i].p_type->s_name,
			p_pool_defs[i].p_type->runtime_id,
			p_pool_defs[i].p_type->size,
			p_pool_defs[i].capacity);
	}

	uint8_t* p_buf = CX_MALLOC(buf_size);

	CX_LOG_FMT(INFO, WORLD, "World buffer size: %"CX_PRI_SIZE" bytes\n", buf_size);
	
	*p_world = (struct cx_world) {
		.p_component_pools = (void*)p_buf,
		.num_component_pools = num_pool_defs,
		.p_buf = p_buf
	};

	size_t cursor = component_pools_array_size;

	for (size_t i = 0; i < num_pool_defs; ++i) {
		struct cx_component_pool* p_pool = p_world->p_component_pools + i;

		void* p_dense_entities_buf = p_buf + cursor;
		cursor += entity_id_size * p_pool_defs[i].capacity;
		cursor = CX_ALIGN_DEFAULT(cursor);
		void* p_dense_components_buf = p_buf + cursor;
		cursor += p_pool_defs[i].p_type->size * p_pool_defs[i].capacity;
		cursor = CX_ALIGN_DEFAULT(cursor);

		*p_pool = (struct cx_component_pool) {
			.p_type = p_pool_defs[i].p_type,
			.p_dense_entities = p_dense_entities_buf,
			.p_dense_components = p_dense_components_buf,
			.capacity = (uint16_t)p_pool_defs[i].capacity,
		};
	};

	for (uint16_t i = 0; i < CX_WORLD_MAX_ENTITIES; ++i) {
		p_world->free_entities[i] = CX_WORLD_MAX_ENTITIES - 1 - i;
	}
	p_world->num_free_entities = CX_WORLD_MAX_ENTITIES;
}

void cx_world_free(struct cx_world* p_world) {
	free(p_world->p_buf);
}

uint16_t cx_world_entity_create(struct cx_world* p_world) {
	CX_ASSERT(p_world->num_free_entities > 0, WORLD);

	uint16_t new_entity_id = p_world->free_entities[--p_world->num_free_entities];

	struct cx_entity* p_new_entity = p_world->entities + new_entity_id;

	p_new_entity->b_alive = 1;
	transform_make_identity(&p_new_entity->transform);

	CX_DBG(CX_LOG_FMT(INFO, WORLD, "Entity created: %u\n", new_entity_id));

	return new_entity_id;
}

void cx_world_entity_destroy(struct cx_world* p_world, uint16_t entity_id) {
	CX_ASSERT_MSG_FMT(entity_id < CX_WORLD_MAX_ENTITIES, WORLD, "Invalid entity id: %u", entity_id);

	for (uint16_t i = 0; i < p_world->num_component_pools; ++i) {
		cx_world_component_remove(p_world, entity_id, p_world->p_component_pools[i].p_type);
	}

	CX_DBG(CX_LOG_FMT(INFO, WORLD, "Entity destroyed: %u\n", entity_id));

	p_world->entities[entity_id].b_alive = 0;
	p_world->free_entities[p_world->num_free_entities++] = entity_id;
}

int cx_world_entity_is_valid(const struct cx_world* p_world, uint16_t entity_id) {
	CX_ASSERT_MSG_FMT(entity_id < CX_WORLD_MAX_ENTITIES, WORLD, "Invalid entity id: %u", entity_id);

	return p_world->entities[entity_id].b_alive;
}

struct transform* cx_world_entity_get_transform(struct cx_world* p_world, uint16_t entity_id) {
	return &p_world->entities[entity_id].transform;
}

const struct transform* cx_world_entity_get_transform_const(const struct cx_world* p_world, uint16_t entity_id) {
	return &p_world->entities[entity_id].transform;
}

const struct cx_component_pool* cx_world_get_component_pool(
	const struct cx_world* p_world,
	const struct cx_component_type* p_type) {

	const size_t pool_id = p_world->component_type_pool_ids[p_type->runtime_id];

	CX_ASSERT_MSG_FMT(pool_id < CX_COMPONENT_MAX_TYPES, WORLD, "No pool for component type '%s'", p_type->s_name);

	return p_world->p_component_pools + pool_id;
}

void* cx_world_component_add(
	struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type) {

	CX_ASSERT_MSG_FMT(entity_id < CX_WORLD_MAX_ENTITIES, WORLD, "Invalid entity id: %u", entity_id);

	const size_t pool_id = p_world->component_type_pool_ids[p_component_type->runtime_id];

	CX_ASSERT_MSG_FMT(pool_id < CX_COMPONENT_MAX_TYPES, WORLD, "No pool for component type '%s'",
		p_component_type->s_name);

	struct cx_component_pool* p_pool = p_world->p_component_pools + pool_id;

	CX_ASSERT_MSG_FMT(p_pool->count < p_pool->capacity, WORLD, "Component pool for type '%s' exhausted",
		p_component_type->s_name);

	size_t dense_index = p_pool->sparse[entity_id];

	if (dense_index < p_pool->count && p_pool->p_dense_entities[dense_index] == entity_id) {
		// already has this component
		return 0;
	}

	p_pool->sparse[entity_id] = p_pool->count;

	p_pool->p_dense_entities[p_pool->sparse[entity_id]] = entity_id;
	
	p_pool->count++;

	CX_DBG(CX_LOG_FMT(INFO, WORLD, "Component added to entity: type='%s', entity=%u\n",
		p_component_type->s_name,
		entity_id));

	return p_pool->p_dense_components + p_component_type->size * p_pool->sparse[entity_id];
}

void cx_world_component_remove(
	struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type) {

	CX_ASSERT_MSG_FMT(entity_id < CX_WORLD_MAX_ENTITIES, WORLD, "Invalid entity id: %u", entity_id);

	const uint16_t pool_id = p_world->component_type_pool_ids[p_component_type->runtime_id];

	CX_ASSERT_MSG_FMT(pool_id < CX_COMPONENT_MAX_TYPES, WORLD, "No pool for component type '%s'",
		p_component_type->s_name);

	struct cx_component_pool* p_pool = p_world->p_component_pools + pool_id;

	const uint16_t dense_index = p_pool->sparse[entity_id];

	if (dense_index < p_pool->count && p_pool->p_dense_entities[dense_index] == entity_id) {
		const size_t last = p_pool->count - 1;
		
		const void* p_moved_component = p_pool->p_dense_components + p_component_type->size * last;
		void* p_removed_component = p_pool->p_dense_components + p_component_type->size * dense_index;
		memcpy(p_removed_component, p_moved_component, p_component_type->size);

		p_pool->p_dense_entities[dense_index] = p_pool->p_dense_entities[last];
		p_pool->sparse[p_pool->p_dense_entities[last]] = dense_index;

		p_pool->count--;

		CX_DBG(CX_LOG_FMT(INFO, WORLD, "Component removed from entity: type='%s', entity=%u\n",
			p_component_type->s_name,
			entity_id));
	}
}

void* cx_world_component_find(
	const struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type) {

	CX_ASSERT_MSG_FMT(entity_id < CX_WORLD_MAX_ENTITIES, WORLD, "Invalid entity id: %u", entity_id);
	
	const uint16_t pool_id = p_world->component_type_pool_ids[p_component_type->runtime_id];

	CX_ASSERT_MSG_FMT(pool_id < CX_COMPONENT_MAX_TYPES, WORLD, "World does not contain a pool for component type '%s'",
		p_component_type->s_name);

	const struct cx_component_pool* p_pool = p_world->p_component_pools + pool_id;

	const uint16_t dense_index = p_pool->sparse[entity_id];

	if (dense_index < p_pool->count && p_pool->p_dense_entities[dense_index] == entity_id) {
		const size_t component_off = p_pool->p_type->size * dense_index;
		return p_pool->p_dense_components + component_off;
	}
	return 0;
}

int cx_world_component_has(
	const struct cx_world* p_world,
	uint16_t entity_id,
	const struct cx_component_type* p_component_type) {
	return cx_world_component_find(p_world, entity_id, p_component_type) != 0;
}

uint16_t cx_world_instantiate_blueprint(struct cx_world* p_world, const struct cx_blueprint* p_blueprint) {
	uint16_t root_entity = 0;

	uint16_t node_entities[CX_WORLD_MAX_ENTITIES];

	for (size_t i = 0; i < p_blueprint->nodes_count; ++i) {
		const struct cx_blueprint_node* p_node = p_blueprint->p_nodes + i;
		
		uint16_t new_entity_id = cx_world_entity_create(p_world);

		node_entities[i] = new_entity_id;

		transform_copy(
			cx_blueprint_node_get_transform(p_blueprint, p_node->id),
			cx_world_entity_get_transform(p_world, new_entity_id));

		void* p_component_data;
		size_t num_components;
		const struct cx_blueprint_node_component* p_components =
			cx_blueprint_node_get_components(p_blueprint, p_node->id, &p_component_data, &num_components);

		for (size_t j = 0; j < num_components; ++j) {
			const void* p_src = (uint8_t*)p_component_data + p_components[j].data_off;
			void* p_new_component = cx_world_component_add(p_world, new_entity_id, p_components[j].p_type);
			memcpy(p_new_component, p_src, p_components[j].p_type->size);
		}
	}

	for (size_t i = 0; i < p_blueprint->nodes_count; ++i) {
		if (p_blueprint->p_nodes[i].parent_id == CX_BLUEPRINT_NODE_INVALID_ID) {
			root_entity = node_entities[i];
			continue;
		}
	
		transform_set_local_transform(
			cx_world_entity_get_transform(p_world, node_entities[i]),
			cx_world_entity_get_transform(p_world, node_entities[p_blueprint->p_nodes[i].parent_id]),
			0);
	}

	return root_entity;
}

void cx_world_compute_transforms(struct cx_world* p_world) {
	for (uint16_t i = 0; i < CX_WORLD_MAX_ENTITIES; ++i) {
		if (p_world->entities[i].b_alive) {
			transform_compute_world_trs_matrix(&p_world->entities[i].transform);
		}
	}
}
