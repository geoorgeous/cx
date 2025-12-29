#ifndef _H__SCENE
#define _H__SCENE

#include "asset.h"
#include "darr.h"
#include "event.h"
#include "object_pool.h"
#include "transform.h"
#include "physics.h"

#define ASSET_TYPE_SCENE 5

struct texture;
struct static_mesh;

struct scene_entity {
    size_t                   _id;
    struct transform         transform;
    asset_handle             p_mesh;
    struct physics_object*   p_physics_object;

};

struct scene_entity_event_data {
    struct scene*        p_scene;
    struct scene_entity* p_entity;
};

struct scene {
    struct object_pool   _entity_pool;
    struct darr          _entities;
    size_t               _next_entity_id;
    struct event         on_new_entity;
    struct event         on_remove_entity;
};

void                 scene_init(struct scene* p_scene);
void                 scene_destroy(struct scene* p_scene);
struct scene_entity* scene_new_entity(struct scene* p_scene);
void                 scene_destroy_entity(struct scene* p_scene, struct scene_entity* p_entity);
struct scene_entity* scene_get_entity(struct scene* p_scene, size_t entity_id);

#endif