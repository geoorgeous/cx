#ifndef SCENE_H
#define SCENE_H

#include "asset.h"
#include "darr.h"
#include "event.h"
#include "object_pool.h"
#include "transform.h"
#include "physics.h"

#define CX_LOG_CAT_SCENE "scene"

#define ASSET_TYPE_SCENE 5

struct texture;
struct static_mesh;

struct scene_entity {
    size_t                   id_;
    struct transform         transform;
    asset_handle             p_mesh;
    struct physics_object*   p_physics_object;

};

struct scene_entity_event_data {
    struct scene*        p_scene;
    struct scene_entity* p_entity;
};

struct scene {
    struct object_pool   entity_pool_;
    struct darr          entities_;
    size_t               next_entity_id_;
    struct event         on_new_entity;
    struct event         on_remove_entity;
};

void                 scene_init(struct scene* p_scene);
void                 scene_destroy(struct scene* p_scene);
struct scene_entity* scene_new_entity(struct scene* p_scene);
void                 scene_destroy_entity(struct scene* p_scene, struct scene_entity* p_entity);
struct scene_entity* scene_get_entity(struct scene* p_scene, size_t entity_id);

#endif
