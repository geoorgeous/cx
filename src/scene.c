#include <stdlib.h>

#include "cx_logging.h"
#include "scene.h"

void scene_init(struct scene* p_scene) {
    *p_scene = (struct scene){0};
    object_pool_init(&p_scene->entity_pool_, sizeof(struct scene_entity), 1024);
    darr_init(&p_scene->entities_, sizeof(struct scene_entity*));

    CX_LOG(TRACE, SCENE, "Scene initialised\n");
}

void scene_destroy(struct scene* p_scene) {
    object_pool_free(&p_scene->entity_pool_);
    darr_free(&p_scene->entities_);
}

struct scene_entity* scene_new_entity(struct scene* p_scene) {
    struct scene_entity* p_new_entity = object_pool_get(&p_scene->entity_pool_);

    if (!p_new_entity) {
        return 0;
    }

    *p_new_entity = (struct scene_entity) {
        .id_ = p_scene->_next_entity_id
    };

    transform_make_identity(&p_new_entity->transform);
    
    struct scene_entity_event_data e = {
        .p_scene = p_scene,
        .p_entity = p_new_entity
    };
    event_broadcast(&p_scene->on_new_entity, &e);

    struct scene_entity** pp_new_entity = darr_push(&p_scene->entities_);
    *pp_new_entity = p_new_entity;
    
    ++p_scene->next_entity_id_;
    
    CX_LOG_FMT(TRACE, SCENE, "Entity created (id=%u)\n", p_new_entity->id_);

    return p_new_entity;
}

void scene_destroy_entity(struct scene* p_scene, struct scene_entity* p_entity) {
    struct scene_entity_event_data e = {
        .p_scene = p_scene,
        .p_entity = p_entity
    };
    event_broadcast(&p_scene->on_new_entity, &e);

    CX_LOG_FMT(TRACE, SCENE, "Entity destroyed (id=%u)\n", p_entity->id_);

    object_pool_return(&p_scene->entity_pool_, p_entity);

    for (size_t i = 0; i < p_scene->entities_.length_; ++i) {
        struct scene_entity** pp_entity = darr_get(&p_scene->entities_, i);
        if (*pp_entity == p_entity) {
            darr_remove(&p_scene->entities_, i);
            break;
        }
    }
}

struct scene_entity* scene_get_entity(struct scene* p_scene, size_t entity_id) {
    for (size_t i = 0; i < p_scene->entities_.length_; ++i) {
        struct scene_entity** pp_entity = darr_get(&p_scene->entities_, i);
        if ((*pp_entity)->id_ == entity_id) {
            return *pp_entity;
        }
    }
    return 0;
}
