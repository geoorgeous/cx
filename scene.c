#include <stdlib.h>

#include "logging.h"
#include "matrix.h"
#include "scene.h"

void scene_init(struct scene* p_scene) {
    *p_scene = (struct scene){0};
    object_pool_init(&p_scene->_entity_pool, sizeof(struct scene_entity), 1024);
    darr_init(&p_scene->_entities, sizeof(struct scene_entity*));

    log_msg(LOG_TRACE, "scene", "Scene initialised\n");
}

void scene_destroy(struct scene* p_scene) {
    object_pool_free(&p_scene->_entity_pool);
    darr_free(&p_scene->_entities);
}

struct scene_entity* scene_new_entity(struct scene* p_scene) {
    struct scene_entity* p_new_entity = object_pool_get(&p_scene->_entity_pool);

    if (!p_new_entity) {
        return 0;
    }

    *p_new_entity = (struct scene_entity) {
        ._id = p_scene->_next_entity_id
    };

    transform_make_identity(&p_new_entity->transform);
    
    struct scene_entity_event_data e = {
        .p_scene = p_scene,
        .p_entity = p_new_entity
    };
    event_broadcast(&p_scene->on_new_entity, &e);

    struct scene_entity** pp_new_entity = darr_push(&p_scene->_entities);
    *pp_new_entity = p_new_entity;
    
    ++p_scene->_next_entity_id;
    
    log_msg(LOG_TRACE, "scene", "Entity created (id=%u)\n", p_new_entity->_id);

    return p_new_entity;
}

void scene_destroy_entity(struct scene* p_scene, struct scene_entity* p_entity) {
    struct scene_entity_event_data e = {
        .p_scene = p_scene,
        .p_entity = p_entity
    };
    event_broadcast(&p_scene->on_new_entity, &e);

    log_msg(LOG_TRACE, "scene", "Entity destroyed (id=%u)\n", p_entity->_id);

    object_pool_return(&p_scene->_entity_pool, p_entity);

    for (size_t i = 0; i < p_scene->_entities._length; ++i) {
        struct scene_entity** pp_entity = darr_get(&p_scene->_entities, i);
        if (*pp_entity == p_entity) {
            darr_remove(&p_scene->_entities, i);
            break;
        }
    }
}

struct scene_entity* scene_get_entity(struct scene* p_scene, size_t entity_id) {
    for (size_t i = 0; i < p_scene->_entities._length; ++i) {
        struct scene_entity** pp_entity = darr_get(&p_scene->_entities, i);
        if ((*pp_entity)->_id == entity_id) {
            return *pp_entity;
        }
    }
    return 0;
}