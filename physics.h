#ifndef _H__PHYSICS
#define _H__PHYSICS

#include <stdbool.h>

#include "darr.h"
#include "object_pool.h"

struct physics_object {
    struct physics_world*    _p_world;
    struct transform*        _p_transform;
    struct physics_collider* _p_collider;
    int                      _b_is_rigidbody;
};

struct physics_rigidbody {
    struct physics_object    base;
    float                    velocity[3];
    float                    force[3];
    float                    mass;
    float                    k_restitution;
    float                    k_static_friction;
    float                    k_dynamic_friction;
};

struct physics_collision_result {
    float a[3];         // Point on object A in world space
    float b[3];         // Point on object B in world space
    float ab_normal[3]; // Collision normal from A -> B
    float depth;        // Penetration depth
};

struct physics_collision {
    struct physics_object*          p_a;
    struct physics_object*          p_b;
    int                             b_has_collision;
    struct physics_collision_result result;
};

enum physics_collider_type {
    PHYSICS_COLLIDER_TYPE_sphere,
    PHYSICS_COLLIDER_TYPE_capsule,
    PHYSICS_COLLIDER_TYPE_hull,
    PHYSICS_COLLIDER_TYPE_plane
};

struct physics_sphere {
    float center[3];
    float radius;
};

struct physics_capsule {
    float p0[3];
    float p1[3];
    float radius;
};

struct physics_hull {
    struct darr verts;
};

struct physics_plane {
    float normal[3];
    float distance;
};

struct physics_collider {
    enum physics_collider_type type;
    union {
        struct physics_sphere  as_sphere;
        struct physics_capsule as_capsule;
        struct physics_hull    as_hull;
        struct physics_plane   as_plane;
    };
    union {
        struct physics_sphere  as_sphere;
        struct physics_capsule as_capsule;
        struct physics_hull    as_hull;
        struct physics_plane   as_plane;
    } _cached;
};

void physics_collider_init(struct physics_collider* p_collider, enum physics_collider_type collider_type);

typedef void(*physics_collision_solver_func)(const struct physics_collision* p_collisions, size_t n, float delta_time);

struct physics_world {
    struct darr        _objects;
    struct darr        _collisions;
    struct darr        _solvers;
    struct object_pool _collider_pool;
    struct object_pool _physics_object_pools[2];
};

void                   physics_world_init(struct physics_world* p_world);
void                   physics_world_destroy(struct physics_world* p_world);
struct physics_object* physics_world_new_object(struct physics_world* p_world, struct transform* p_transform, int b_is_rigidbody);
void                   physics_world_destroy_object(struct physics_world* p_world, struct physics_object* p_object);
void                   physics_world_new_object_collider(struct physics_world* p_world, struct physics_object* p_object, enum physics_collider_type type);
void                   physics_world_destroy_object_collider(struct physics_world* p_world, struct physics_object* p_object);
void                   physics_world_add_solver(struct physics_world* p_world, physics_collision_solver_func p_solver_func);
void                   physics_world_remove_solver(struct physics_world* p_world, physics_collision_solver_func p_solver_func);
void                   physics_world_step(struct physics_world* p_world, float delta_time);

int physics_test_collision(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_sphere_sphere(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_sphere_capsule(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_sphere_hull(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_sphere_plane(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_capsule_capsule(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_capsule_hull(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_capsule_plane(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_hull_hull(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);

int physics_test_collision_hull_plane(
    const struct physics_collider* p_a,
    const struct physics_collider* p_b,
    struct physics_collision_result* p_result);
    
void physics_collision_solver_impulse(const struct physics_collision* p_collisions, size_t n, float delta_time);
void physics_collision_solver_smooth_positions(const struct physics_collision* p_collisions, size_t n, float delta_time);

#endif