#include <math.h>

#include "logging.h"
#include "math_utils.h"
#include "matrix.h"
#include "physics.h"
#include "transform.h"
#include "vector.h"

// https://github.com/IainWinter/IwEngine/blob/3e2052855fea85718b7a499a7b1a3befd49d812b/IwEngine/include/iw/physics/impl/TestCollision.h

#define PHYSICS_MAX_COLLIDERS      1024
#define PHYSICS_MAX_STATIC_OBJECTS 1024
#define PHYSICS_MAX_RIGIDBODIES    512

static void physics_world_step_rigidbodies(struct physics_world* p_world, float delta_time);
static void physics_world_detect_collisions(struct physics_world* p_world);
static void physics_world_detect_collisions_broadphase(struct physics_world* p_world);
static void physics_world_detect_collisions_narrowphase(struct physics_world* p_world);
static void physics_world_resolve_collisions(struct physics_world* p_world, const struct physics_collision* p_collisions, size_t n, float delta_time);

static void physics_collider_apply_transform(struct physics_collider* p_collider, const struct transform* p_t);
static void physics_sphere_apply_transform(struct physics_collider* p_collider, const struct transform* p_t);
static void physics_capsule_apply_transform(struct physics_collider* p_collider, const struct transform* p_t);
static void physics_hull_apply_transform(struct physics_collider* p_collider, const struct transform* p_t);
static void physics_plane_apply_transform(struct physics_collider* p_collider, const struct transform* p_t);
static void physics_collider_undo_transform(struct physics_collider* p_collider);
static void physics_sphere_undo_transform(struct physics_collider* p_collider);
static void physics_capsule_undo_transform(struct physics_collider* p_collider);
static void physics_hull_undo_transform(struct physics_collider* p_collider);
static void physics_plane_undo_transform(struct physics_collider* p_collider);

static int physics_test_sphere_sphere_internal(const float* p_center_a, float radius_a, const float* p_center_b, float radius_b, struct physics_collision_result* p_result);
static int physics_test_gjk(const struct physics_collider* p_a, const struct physics_collider* p_b, struct physics_collision_result* p_result);

int  gjk(const struct physics_collider* p_a, const struct physics_collider* p_b, float simplex[4][3]);
void gjk_find_extreme(const struct physics_collider* p_collider, const float* p_dir, float* p_extreme);
void gjk_find_extreme_on_sphere(const struct physics_sphere* p_sphere, const float* p_dir, float* p_extreme);
void gjk_find_extreme_on_capsule(const struct physics_capsule* p_capsule, const float* p_dir, float* p_extreme);
void gjk_find_extreme_on_hull(const struct physics_hull* p_hull, const float* p_dir, float* p_extreme);
void gjk_find_support(const struct physics_collider* p_a, const struct physics_collider* p_b, const float* p_dir, float* p_support);
int  gjk_process_simplex(float simplex[4][3], int* p_simplex_d, float* p_dir);
void gjk_process_simplex_line(float simplex[4][3], int* p_simplex_d, float* p_dir);
void gjk_process_simplex_triangle(float simplex[4][3], int* p_simplex_d, float* p_dir);
int  gjk_process_simplex_tetrahedron(float simplex[4][3], int* p_simplex_d, float* p_dir);
void epa(float simplex[4][3], const struct physics_collider* p_a, const struct physics_collider* p_b, struct physics_collision_result* p_result);

void physics_collider_init(struct physics_collider* p_collider, enum physics_collider_type collider_type) {
	p_collider->type = collider_type;
	switch (p_collider->type) {
		case PHYSICS_COLLIDER_TYPE_sphere: {
			vec3_clr(p_collider->as_sphere.center);
			p_collider->as_sphere.radius = 0.5f;
			break;
		}

		case PHYSICS_COLLIDER_TYPE_capsule: {
			p_collider->as_capsule.p0[0] =
			p_collider->as_capsule.p0[2] =
			p_collider->as_capsule.p1[0] =
			p_collider->as_capsule.p1[2] =  0;
			p_collider->as_capsule.p0[1] = -0.5f;
			p_collider->as_capsule.p1[1] =  0.5f;
			p_collider->as_capsule.radius = 0.15f;
			break;
		}

		case PHYSICS_COLLIDER_TYPE_hull: {
			const float elemsize = sizeof(float) * 3;
			darr_init(&p_collider->as_hull.verts, elemsize);
			darr_init(&p_collider->_cached.as_hull.verts, elemsize);
			break;
		}

		case PHYSICS_COLLIDER_TYPE_plane: {
			p_collider->as_plane.normal[0] = 0;
			p_collider->as_plane.normal[1] = 1;
			p_collider->as_plane.normal[2] = 0;
			p_collider->as_plane.distance =  0;
			break;
		}
	}
}

void physics_world_init(struct physics_world* p_world) {
	darr_init(&p_world->_objects, sizeof(struct physics_object*));
	darr_init(&p_world->_collisions, sizeof(struct physics_collision));
	darr_init(&p_world->_solvers, sizeof(physics_collision_solver_func));
	object_pool_init(&p_world->_collider_pool, sizeof(struct physics_collider), PHYSICS_MAX_COLLIDERS);
	object_pool_init(&p_world->_physics_object_pools[0], sizeof(struct physics_object), PHYSICS_MAX_STATIC_OBJECTS);
	object_pool_init(&p_world->_physics_object_pools[1], sizeof(struct physics_rigidbody), PHYSICS_MAX_RIGIDBODIES);

	cx_log(CX_LOG_TRACE, "physics", "Physics world initialised\n");
}

void physics_world_destroy(struct physics_world* p_world) {
	darr_free(&p_world->_objects);
	darr_free(&p_world->_collisions);
	darr_free(&p_world->_solvers);
	object_pool_free(&p_world->_collider_pool);
	object_pool_free(&p_world->_physics_object_pools[0]);
	object_pool_free(&p_world->_physics_object_pools[1]);
}

struct physics_object* physics_world_new_object(struct physics_world* p_world, struct transform* p_transform, int b_is_rigidbody) {
	struct physics_object* p_object = object_pool_get(&p_world->_physics_object_pools[b_is_rigidbody]);
	*p_object = (struct physics_object) {
		._p_world = p_world,
		._p_transform = p_transform,
		._b_is_rigidbody = b_is_rigidbody
	};

	if (b_is_rigidbody) {
		struct physics_rigidbody* p_rigidbody = (struct physics_rigidbody*)p_object;
		
		vec3_clr(p_rigidbody->velocity);
		vec3_clr(p_rigidbody->force);
		p_rigidbody->mass = 1;
		p_rigidbody->k_restitution = 0.5f;
		p_rigidbody->k_static_friction = 1.0f;
		p_rigidbody->k_dynamic_friction = 0.15f;
	}

	struct physics_object** pp_object = darr_push(&p_world->_objects);
	*pp_object = p_object;

	cx_log_fmt(CX_LOG_TRACE, "physics", "%s created\n", b_is_rigidbody ? "Rigidbody" : "Static body");

	return p_object;
}

void physics_world_destroy_object(struct physics_world* p_world, struct physics_object* p_object) {
	for (size_t i = 0; i < p_world->_objects._length; ++i) {
		struct physics_object** pp_object = darr_get(&p_world->_objects, i);
		if (*pp_object == p_object) {
			darr_remove(&p_world->_objects, i);
			break;
		}
	}

	cx_log_fmt(CX_LOG_TRACE, "physics", "%s destroyed\n", p_object->_b_is_rigidbody ? "Rigidbody" : "Static object");

	if (p_object->_p_collider) {
		object_pool_return(&p_world->_collider_pool, p_object->_p_collider);
	}
	
	object_pool_return(&p_world->_physics_object_pools[p_object->_b_is_rigidbody], p_object);
}


void physics_world_new_object_collider(struct physics_world* p_world, struct physics_object* p_object, enum physics_collider_type type) {
	if (p_object->_p_collider) {
		return;
	}

	p_object->_p_collider = object_pool_get(&p_world->_collider_pool);
	physics_collider_init(p_object->_p_collider, type);

	cx_log_fmt(CX_LOG_TRACE, "physics", "Collider added to %s (type=%d)\n", p_object->_b_is_rigidbody ? "rigidbody" : "static body", type);
}

void physics_world_destroy_object_collider(struct physics_world* p_world, struct physics_object* p_object) {
	if (!p_object->_p_collider) {
		return;
	}

	cx_log_fmt(CX_LOG_TRACE, "physics", "Collider removed from %s (type=%d)\n", p_object->_b_is_rigidbody ? "rigidbody" : "static body", p_object->_p_collider->type);

	if (p_object->_p_collider->type == PHYSICS_COLLIDER_TYPE_hull) {
		darr_free(&p_object->_p_collider->as_hull.verts);
	}

	object_pool_return(&p_world->_collider_pool, p_object->_p_collider);
	p_object->_p_collider = 0;
}

void physics_world_add_solver(struct physics_world* p_world, physics_collision_solver_func p_solver_func) {
	physics_collision_solver_func* p_solver = darr_push(&p_world->_solvers);
	*p_solver = p_solver_func;
}

void physics_world_remove_solver(struct physics_world* p_world, physics_collision_solver_func p_solver_func) {
	for (size_t i = 0; i < p_world->_solvers._length; ++i) {
		physics_collision_solver_func* p = darr_get(&p_world->_solvers, i);
		if (*p == p_solver_func) {
			darr_remove(&p_world->_solvers, i);
			break;
		}
	}
}

void physics_world_step(struct physics_world* p_world, float delta_time) {
	//CX_DBG_LOG_FMT("physics", "Step (delta_time=%f)\n", delta_time);

	if (FLT_CMP(delta_time, 0)) {
		return;
	}

	physics_world_step_rigidbodies(p_world, delta_time);
	physics_world_detect_collisions(p_world);
	physics_world_resolve_collisions(p_world, p_world->_collisions._p_buffer, p_world->_collisions._length, delta_time);
}

void physics_world_step_rigidbodies(struct physics_world* p_world, float delta_time) {
	const float gravity_force[] = { 0, -9.81f, 0 };
	float temp[3];

	for (size_t i = 0; i < p_world->_objects._length; ++i) {
		struct physics_object* p_object = *(struct physics_object**)darr_get(&p_world->_objects, i);

		if (!p_object->_b_is_rigidbody) {
			continue;
		}

		struct physics_rigidbody* p_rigidbody = (struct physics_rigidbody*)p_object;

		vec3_mul_s(gravity_force, p_rigidbody->mass, temp);
		vec3_add(p_rigidbody->force, temp, p_rigidbody->force);

		vec3_div_s(p_rigidbody->force, p_rigidbody->mass, temp);
		vec3_mul_s(temp, delta_time, temp);
		vec3_add(p_rigidbody->velocity, temp, p_rigidbody->velocity);

		float new_position[3];
		vec3_mul_s(p_rigidbody->velocity, delta_time, temp);
		vec3_add(p_object->_p_transform->world_position, temp, new_position);
		transform_set_world_position(p_object->_p_transform, new_position);

		vec3_clr(p_rigidbody->force);
	}
}

void physics_collider_apply_transform(struct physics_collider* p_collider, const struct transform* p_t) {
	static void(*const func_table[])(struct physics_collider*, const struct transform*) = {
		physics_sphere_apply_transform,
		physics_capsule_apply_transform,
		physics_hull_apply_transform,
		physics_plane_apply_transform
	};
	func_table[p_collider->type](p_collider, p_t);
}

void physics_sphere_apply_transform(struct physics_collider* p_collider, const struct transform* p_t) {
	p_collider->_cached.as_sphere = p_collider->as_sphere;

	vec3_add(p_collider->as_sphere.center, p_t->world_position, p_collider->as_sphere.center);
	p_collider->as_sphere.radius *= vec3_major(p_t->world_scale);
}

void physics_capsule_apply_transform(struct physics_collider* p_collider, const struct transform* p_t) {
	p_collider->_cached.as_capsule = p_collider->as_capsule;
	// todo:
}

void physics_hull_apply_transform(struct physics_collider* p_collider, const struct transform* p_t) {
	darr_set_length(&p_collider->_cached.as_hull.verts, p_collider->as_hull.verts._length);
	for (size_t i = 0; i < p_collider->as_hull.verts._length; ++i) {
		float* p_v = darr_get(&p_collider->as_hull.verts, i);
		float* p_v_cached = darr_get(&p_collider->_cached.as_hull.verts, i);
		vec3_set(p_v, p_v_cached);
		matrix_multiply_vec3(p_t->world_trs_matrix, p_v, p_v);
	}
}

void physics_plane_apply_transform(struct physics_collider* p_collider, const struct transform* p_t) {
	p_collider->_cached.as_plane = p_collider->as_plane;

	quaternion_rotate_vec3(p_t->world_rotation, p_collider->as_plane.normal, p_collider->as_plane.normal);

	p_collider->as_plane.distance += vec3_len(p_t->world_position) * signf(vec3_dot(p_collider->as_plane.normal, p_t->world_position));
}

void physics_collider_undo_transform(struct physics_collider* p_collider) {
	static void(*const func_table[])(struct physics_collider*) = {
		physics_sphere_undo_transform,
		physics_capsule_undo_transform,
		physics_hull_undo_transform,
		physics_plane_undo_transform
	};
	func_table[p_collider->type](p_collider);
}

void physics_sphere_undo_transform(struct physics_collider* p_collider) {
	p_collider->as_sphere = p_collider->_cached.as_sphere;
}

void physics_capsule_undo_transform(struct physics_collider* p_collider) {
	p_collider->as_capsule = p_collider->_cached.as_capsule;
}

void physics_hull_undo_transform(struct physics_collider* p_collider) {
	for (size_t i = 0; i < p_collider->as_hull.verts._length; ++i) {
		float* p_v = darr_get(&p_collider->as_hull.verts, i);
		float* p_v_cached = darr_get(&p_collider->_cached.as_hull.verts, i);
		vec3_set(p_v_cached, p_v);
	}
}

void physics_plane_undo_transform(struct physics_collider* p_collider) {
	p_collider->as_plane = p_collider->_cached.as_plane;
}

void physics_world_detect_collisions(struct physics_world* p_world) {
	for (size_t i = 0; i < p_world->_objects._length; ++i) {
		struct physics_object* p_object = *(struct physics_object**)darr_get(&p_world->_objects, i);

		if (!p_object->_p_collider) {
			continue;
		}

		physics_collider_apply_transform(p_object->_p_collider, p_object->_p_transform);
	}

	physics_world_detect_collisions_broadphase(p_world);
	physics_world_detect_collisions_narrowphase(p_world);
	
	for (size_t i = 0; i < p_world->_objects._length; ++i) {
		struct physics_object* p_object = *(struct physics_object**)darr_get(&p_world->_objects, i);

		if (!p_object->_p_collider) {
			continue;
		}

		physics_collider_undo_transform(p_object->_p_collider);
	}
}

void physics_world_detect_collisions_broadphase(struct physics_world* p_world) {
	// todo: AABB tree
	// AABB intersection
	darr_set_length(&p_world->_collisions, 0);
	
	for (size_t a = 0; a < p_world->_objects._length; ++a) {
		for (size_t b = 0; b < p_world->_objects._length; ++b) {
			if (a == b) {
				break;
			}

			struct physics_object* p_a = *(struct physics_object**)darr_get(&p_world->_objects, a);
			struct physics_object* p_b = *(struct physics_object**)darr_get(&p_world->_objects, b);

			if (!p_a->_p_collider || !p_b->_p_collider) {
				continue;
			}

			struct physics_collision* p_collision = darr_push(&p_world->_collisions);
			*p_collision = (struct physics_collision) {
				.p_a = p_a,
				.p_b = p_b
			};
		}
	}
}

void physics_world_detect_collisions_narrowphase(struct physics_world* p_world) {
	for (int i = p_world->_collisions._length - 1; i >= 0; --i) {
		struct physics_collision* p_collision = darr_get(&p_world->_collisions, i);

		p_collision->b_has_collision = physics_test_collision(p_collision->p_a->_p_collider, p_collision->p_b->_p_collider, &p_collision->result);
		
		if (!p_collision->b_has_collision) {
			darr_remove(&p_world->_collisions, i);
		}
	}
}

void physics_world_resolve_collisions(struct physics_world* p_world, const struct physics_collision* p_collisions, size_t n, float delta_time) {
	for (size_t i = 0; i < p_world->_solvers._length; ++i) {
		physics_collision_solver_func* p_solver = darr_get(&p_world->_solvers, i);
		(*p_solver)(p_collisions, n, delta_time);
	}
}

// COLLISION TESTS

typedef int(*physics_test_collision_func)(
	const struct physics_collider*,
	const struct physics_collider*,
	struct physics_collision_result*);

int physics_test_collision(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	static const physics_test_collision_func test_func_table[3][4] = {
		{ (void*)physics_test_collision_sphere_sphere, (void*)physics_test_collision_sphere_capsule,  (void*)physics_test_collision_sphere_hull,  (void*)physics_test_collision_sphere_plane  },
		{ 0,                                           (void*)physics_test_collision_capsule_capsule, (void*)physics_test_collision_capsule_hull, (void*)physics_test_collision_capsule_plane },
		{ 0,                                           0,                                             (void*)physics_test_collision_hull_hull,    (void*)physics_test_collision_hull_plane    }
	};

	if (p_a->type == PHYSICS_COLLIDER_TYPE_plane && p_b->type == PHYSICS_COLLIDER_TYPE_plane) {
		*p_result = (struct physics_collision_result){0};
		return 0;
	}

	const int b_swap = p_a->type > p_b->type;

	if (b_swap) {
		const void* p_temp;

		p_temp = p_a;
		p_a = p_b;
		p_b = p_temp;
	}

	const int b_has_collision = test_func_table[p_a->type][p_b->type](p_a, p_b, p_result);
	if (b_has_collision && b_swap) {
		float v_temp[3];
		vec3_set(p_result->a, v_temp);
		vec3_set(p_result->b, p_result->a);
		vec3_set(v_temp, p_result->b);
		vec3_inv(p_result->ab_normal, p_result->ab_normal);
	}

	return b_has_collision;
}

int physics_test_sphere_sphere_internal(const float* p_center_a, float radius_a, const float* p_center_b, float radius_b, struct physics_collision_result* p_result) {
	float ab[3];
	vec3_sub(p_center_b, p_center_a, ab);
	
	const float radius_ab = radius_a + radius_b;
	
	float d = vec3_dot(ab, ab);

	if (d > radius_ab * radius_ab) {
		*p_result = (struct physics_collision_result){0};
		// CX_DBG_LOG_FMT("physics", "Sphere-sphere no collision: a=([%f, %f, %f], %f) b=([%f, %f, %f], %f), dist=%.12f, a.radius+b.radius=%.12f, dist<(radius_ab*radius_ab)=%u\n"
		//     , p_center_a[0], p_center_a[1], p_center_a[2], radius_a
		//     , p_center_b[0], p_center_b[1], p_center_b[2], radius_b
		//     , d
		//     , radius_a + radius_b
		//     , d < radius_ab * radius_ab);
		return 0;
	}

	d = sqrtf(d);

	p_result->depth = radius_ab - d;

	vec3_div_s(ab, d, p_result->ab_normal);

	vec3_mul_s(p_result->ab_normal, radius_a, p_result->a);
	vec3_add(p_result->a, p_center_a, p_result->a);
	
	vec3_mul_s(p_result->ab_normal, radius_b, p_result->b);
	vec3_sub(p_result->b, p_center_b, p_result->b);
	
	// CX_DBG_LOG_FMT("physics", "Sphere-sphere collision detected: a=([%f, %f, %f], %f) b=([%f, %f, %f], %f), dist=%.12f, a.radius+b.radius=%.12f, dist<(a.radius+b.radius)=%u, result.a=[%f, %f, %f], result.b=[%f, %f, %f], result.norm=[%f, %f, %f], result.depth=%f\n"
	//     , p_center_a[0], p_center_a[1], p_center_a[2], radius_a
	//     , p_center_b[0], p_center_b[1], p_center_b[2], radius_b
	//     , d
	//     , radius_a + radius_b
	//     , d < (radius_a + radius_b)
	//     , p_result->a[0], p_result->a[1], p_result->a[2]
	//     , p_result->b[0], p_result->b[1], p_result->b[2]
	//     , p_result->ab_normal[0], p_result->ab_normal[1], p_result->ab_normal[2]
	//     , p_result->depth);

	return 1;
}

int physics_test_collision_sphere_sphere(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	return physics_test_sphere_sphere_internal(p_a->as_sphere.center, p_a->as_sphere.radius, p_b->as_sphere.center, p_b->as_sphere.radius, p_result);
}

int physics_test_collision_sphere_capsule(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	float v0[3];
	float v1[3];

	vec3_sub(p_a->as_sphere.center, p_b->as_capsule.p0, v0);
	vec3_sub(p_b->as_capsule.p1, p_b->as_capsule.p0, v1);
	
	const float s0 = vec3_dot(v0, v1);
	const float s1 = vec3_dot(v1, v1);
	const float s2 = FLT_CMP(s1, 0) ? -1 : s0 / s1;

	if (s2 < 0) {
		vec3_sub(p_a->as_sphere.center, p_b->as_capsule.p0, v0);
	} else if (s2 > 1) {
		vec3_sub(p_a->as_sphere.center, p_b->as_capsule.p1, v0);
	} else {
		vec3_mul_s(v1, s2, v1);
		vec3_add(v0, v1, v0);
	}
	
	return physics_test_sphere_sphere_internal(p_a->as_sphere.center, p_a->as_sphere.radius, v0, p_b->as_capsule.radius, p_result);
}

int physics_test_collision_sphere_hull(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	return physics_test_gjk(p_a, p_b, p_result);
}

int physics_test_collision_sphere_plane(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	float v0[3];

	vec3_mul_s(p_b->as_plane.normal, p_b->as_plane.distance, v0);

	vec3_sub(p_a->as_sphere.center, v0, v0);

	const float d = vec3_dot(v0, p_b->as_plane.normal);

	if (d > p_a->as_sphere.radius) {
		return 0;
	}

	vec3_inv(p_b->as_plane.normal, p_result->ab_normal);

	vec3_mul_s(p_result->ab_normal, d, p_result->a);
	vec3_add(p_result->a, p_a->as_sphere.center, p_result->a);

	vec3_mul_s(p_result->ab_normal, p_a->as_sphere.radius, p_result->b);
	vec3_add(p_result->b, p_a->as_sphere.center, p_result->b);
	
	p_result->depth = p_a->as_sphere.radius - d;

	// CX_DBG_LOG_FMT("physics", "Sphere-plane collision detected: a=([%f, %f, %f], %f) b=([%f, %f, %f], %f), dist=%.12f, result.a=[%f, %f, %f], result.b=[%f, %f, %f], result.norm=[%f, %f, %f], result.depth=%f\n"
	//     , center_a[0], center_a[1], center_a[2], radius_a
	//     , normal_b[0], normal_b[1], normal_b[2], p_b->distance
	//     , distance
	//     , p_result->a[0], p_result->a[1], p_result->a[2]
	//     , p_result->b[0], p_result->b[1], p_result->b[2]
	//     , p_result->normal[0], p_result->normal[1], p_result->normal[2]
	//     , p_result->depth);
	
	return 1;
}

int physics_test_collision_capsule_capsule(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	*p_result = (struct physics_collision_result){0};
	return 0;

	// todo: capsule-capsule collision
}

int physics_test_collision_capsule_hull(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	return physics_test_gjk(p_a, p_b, p_result);
}

int physics_test_collision_capsule_plane(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	*p_result = (struct physics_collision_result){0};
	return 0;

	// todo: capsule-plane collision
}

int physics_test_collision_hull_hull(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	return physics_test_gjk(p_a, p_b, p_result);
}

int physics_test_collision_hull_plane(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	vec3_mul_s(p_b->as_plane.normal, p_b->as_plane.distance, p_result->b);
	
	vec3_inv(p_b->as_plane.normal, p_result->ab_normal);

	gjk_find_extreme_on_hull(&p_a->as_hull, p_result->ab_normal, p_result->a);

	float ba[3];
	vec3_sub(p_result->b, p_result->a, ba);

	p_result->depth = vec3_dot(ba, p_b->as_plane.normal);

	if (p_result->depth < 0) {
		*p_result = (struct physics_collision_result){0};
		return 0;
	}

	return 1;
}

// SOLVERS

void physics_collision_solver_impulse(const struct physics_collision* p_collisions, size_t n, float delta_time) {
	for (size_t i = 0; i < n; ++i) {
		struct physics_rigidbody* p_rb_a = (struct physics_rigidbody*)(p_collisions[i].p_a->_b_is_rigidbody ? p_collisions[i].p_a : 0);
		struct physics_rigidbody* p_rb_b = (struct physics_rigidbody*)(p_collisions[i].p_b->_b_is_rigidbody ? p_collisions[i].p_b : 0);

		float vel_a[3] = {0};
		float vel_b[3] = {0};

		if (p_rb_a) {
			vec3_set(p_rb_a->velocity, vel_a);
		}
		
		if (p_rb_b) {
			vec3_set(p_rb_b->velocity, vel_b);
		}

		float rel_vel[3];
		vec3_sub(vel_b, vel_a, rel_vel);

		float n_spd = vec3_dot(rel_vel, p_collisions[i].result.ab_normal);

		if (!(n_spd < 0)) {
			continue;
		}

		const float invmass_a = p_rb_a && !FLT_CMP(p_rb_a->mass, 0) ? 1.0f / p_rb_a->mass : 1;
		const float invmass_b = p_rb_b && !FLT_CMP(p_rb_b->mass, 0) ? 1.0f / p_rb_b->mass : 1;

		if (FLT_CMP(invmass_a, 0) && FLT_CMP(invmass_b, 0)) {
			cx_log(CX_LOG_WARNING, "physics", "Zero-mass collision detected.\n");
			continue;
		}

		// Velocity impulse

		const float e = (p_rb_a ? p_rb_a->k_restitution : 1.0f) * (p_rb_b ? p_rb_b->k_restitution : 1.0f);
		const float j = -(2.0f + e) * n_spd / (invmass_a + invmass_b);

		float impulse[3];
		vec3_mul_s(p_collisions[i].result.ab_normal, j, impulse);

		float impulse_a[3];
		vec3_mul_s(impulse, invmass_a, impulse_a);
		vec3_sub(vel_a, impulse_a, vel_a);
		
		float impulse_b[3];
		vec3_mul_s(impulse, invmass_b, impulse_b);
		vec3_add(vel_b, impulse_b, vel_b);

		// Friction

		vec3_sub(vel_b, vel_a, rel_vel);
		
		n_spd = vec3_dot(rel_vel, p_collisions[i].result.ab_normal);

		float tangent[3];
		vec3_mul_s(p_collisions[i].result.ab_normal, n_spd, tangent);
		vec3_sub(rel_vel, tangent, tangent);

		if (!vec3_is_zero(tangent)) {
			vec3_norm(tangent, tangent);
		}

		const float fvel = vec3_dot(rel_vel, tangent);

		float friction_force[3];
		float ab_friction_k[2];
		float mu;

		const float f = -fvel / (invmass_a + invmass_b);

		ab_friction_k[0] = p_rb_a ? p_rb_a->k_static_friction : 0;
		ab_friction_k[1] = p_rb_b ? p_rb_b->k_static_friction : 0;
		mu = vec_len(2, ab_friction_k);

		if (fabsf(f) < j * mu) {
			vec3_mul_s(tangent, f, friction_force);
		} else {
			ab_friction_k[0] = p_rb_a ? p_rb_a->k_dynamic_friction : 0;
			ab_friction_k[1] = p_rb_b ? p_rb_b->k_dynamic_friction : 0;
			mu = vec_len(2, ab_friction_k);

			vec3_mul_s(tangent, -j * mu, friction_force);
		}

		// Combine, apply

		if (p_rb_a) {
			float new_velocity[3];
			vec3_mul_s(friction_force, invmass_a, new_velocity);
			vec3_sub(vel_a, friction_force, p_rb_a->velocity);
		}

		if (p_rb_b) {
			float new_velocity[3];
			vec3_mul_s(friction_force, invmass_b, new_velocity);
			vec3_add(vel_b, friction_force, p_rb_b->velocity);
		}
	}
}

void physics_collision_solver_smooth_positions(const struct physics_collision* p_collisions, size_t n, float delta_time) {
	static struct darr deltas;
	if (deltas._element_size == 0) {
		darr_init(&deltas, sizeof(float) * 6);
	}

	for (size_t i = 0; i < n; ++i) {
		struct physics_rigidbody* p_rb_a = (struct physics_rigidbody*)(p_collisions[i].p_a->_b_is_rigidbody ? p_collisions[i].p_a : 0);
		struct physics_rigidbody* p_rb_b = (struct physics_rigidbody*)(p_collisions[i].p_b->_b_is_rigidbody ? p_collisions[i].p_b : 0);

		float* p_deltas = (float*)darr_push(&deltas);
		float* p_delta_a = &p_deltas[0];
		float* p_delta_b = &p_deltas[3];
		
		vec3_clr(p_delta_a);
		vec3_clr(p_delta_b);

		const float invmass_a = p_rb_a && !FLT_CMP(p_rb_a->mass, 0) ? 1.0f / p_rb_a->mass : 0;
		const float invmass_b = p_rb_b && !FLT_CMP(p_rb_b->mass, 0) ? 1.0f / p_rb_b->mass : 0;

		if (FLT_CMP(invmass_a, 0) && FLT_CMP(invmass_b, 0)) {
			cx_log(CX_LOG_WARNING, "physics", "Zero-mass collision detected.\n");
			continue;
		}

		const float percent = 0.8f;

		float correction[3];
		float correction_x = percent * fmaxf(p_collisions[i].result.depth, 0) / (invmass_a + invmass_b);
		vec3_mul_s(p_collisions[i].result.ab_normal, correction_x, correction);

		if (p_rb_a) {
			vec3_mul_s(correction, invmass_a, p_delta_a);
		}
		
		if (p_rb_b) {
			vec3_mul_s(correction, invmass_b, p_delta_b);
		}
	}
	
	for (size_t i = 0; i < n; ++i) {
		struct physics_rigidbody* p_rb_a = (struct physics_rigidbody*)(p_collisions[i].p_a->_b_is_rigidbody ? p_collisions[i].p_a : 0);
		struct physics_rigidbody* p_rb_b = (struct physics_rigidbody*)(p_collisions[i].p_b->_b_is_rigidbody ? p_collisions[i].p_b : 0);
		
		const float* p_deltas = darr_get(&deltas, i);

		//CX_DBG_LOG_FMT(0, "Smooth positions solver: delta_a=[%f, %f, %f], delta_b=[%f, %f, %f]\n", p_deltas[0], p_deltas[1], p_deltas[2], p_deltas[3], p_deltas[4], p_deltas[5]);

		if (p_rb_a) {
			float new_position[3];
			vec3_sub(p_rb_a->base._p_transform->world_position, &p_deltas[0], new_position);

			transform_set_world_position(p_rb_a->base._p_transform, new_position);
		}

		if (p_rb_b) {
			float new_position[3];
			vec3_add(p_rb_b->base._p_transform->world_position, &p_deltas[3], new_position);

			transform_set_world_position(p_rb_b->base._p_transform, new_position);
		}
	}

	darr_set_length(&deltas, 0);
}

// GJK

int physics_test_gjk(
	const struct physics_collider* p_a,
	const struct physics_collider* p_b,
	struct physics_collision_result* p_result) {
	float simplex[4][3] = {0};
	int b_collision = gjk(p_a, p_b, simplex);

	if (b_collision) {
		epa(simplex, p_a, p_b, p_result);
	} else {
		*p_result = (struct physics_collision_result){0};
	}

	return b_collision;
}

typedef void(*physics_collider_find_extreme_func)(const void*, const float*, float*);

#define GJK_SAME_SIDE(A, B) (vec3_dot(A, B) > 0)

int gjk(const struct physics_collider* p_a, const struct physics_collider* p_b, float simplex[4][3]) {
	int   simplex_d = 1;

	// Arbitrary initial search direction
	float dir[3] = {1, 0, 0};

	// Find our first point
	gjk_find_support(p_a, p_b, dir, simplex[0]);

	// Now our search direction is opposite to our first point
	vec3_inv(simplex[0], dir);

	while(true) {
		// Find the next point for our simplex
		gjk_find_support(p_a, p_b, dir, simplex[simplex_d]);
		++simplex_d;

		// if (simplex_d == 2) {
		// 	CX_DBG_LOG_FMT("gjk", "new simplex=(\n\tB[%f, %f, %f]\n\tA[%f, %f, %f]\n), dir=[%f, %f, %f]\n", simplex[0][0], simplex[0][1], simplex[0][2], simplex[1][0], simplex[1][1], simplex[1][2], dir[0], dir[1], dir[2]);
		// } else if (simplex_d == 3) {
		// 	CX_DBG_LOG_FMT("gjk", "new simplex=(\n\tC[%f, %f, %f]\n\tB[%f, %f, %f]\n\tA[%f, %f, %f]\n), dir=[%f, %f, %f]\n", simplex[0][0], simplex[0][1], simplex[0][2], simplex[1][0], simplex[1][1], simplex[1][2], simplex[2][0], simplex[2][1], simplex[2][2], dir[0], dir[1], dir[2]);
		// } else {
		// 	CX_DBG_LOG_FMT("gjk", "new simplex=(\n\tD[%f, %f, %f]\n\tC[%f, %f, %f]\n\tB[%f, %f, %f]\n\tA[%f, %f, %f]\n), dir=[%f, %f, %f]\n", simplex[0][0], simplex[0][1], simplex[0][2], simplex[1][0], simplex[1][1], simplex[1][2], simplex[2][0], simplex[2][1], simplex[2][2], simplex[3][0], simplex[3][1], simplex[3][2], dir[0], dir[1], dir[2]);
		// }

		// If the new point is not beyond the origin from the perspective of the search direction,
		// then there's no collision!
		if (!GJK_SAME_SIDE(simplex[simplex_d - 1], dir)) {
			//CX_DBG_LOG("gjk", "NO COLLISION\n");
			return 0;
		}

		if (gjk_process_simplex(simplex, &simplex_d, dir)) {
			//CX_DBG_LOG("gjk", "\tCOLLISION DETECTED\n");
			return 1;
		}
	}
}

void gjk_find_extreme(const struct physics_collider* p_collider, const float* p_dir, float* p_extreme) {
	static const physics_collider_find_extreme_func func_table[] = {
		(void*)gjk_find_extreme_on_sphere,
		(void*)gjk_find_extreme_on_capsule,
		(void*)gjk_find_extreme_on_hull
	};
	func_table[p_collider->type]((const void*)&p_collider->as_sphere, p_dir, p_extreme);
}

void gjk_find_extreme_on_sphere(const struct physics_sphere* p_sphere, const float* p_dir, float* p_extreme) {
	vec3_norm(p_dir, p_extreme);
	vec3_mul_s(p_extreme, p_sphere->radius, p_extreme);
	vec3_add(p_extreme, p_sphere->center, p_extreme);
}

void gjk_find_extreme_on_capsule(const struct physics_capsule* p_capsule, const float* p_dir, float* p_extreme) {
	// todo: find extreme of capsule
}

void gjk_find_extreme_on_hull(const struct physics_hull* p_hull, const float* p_dir, float* p_extreme) {
	float dot_max = -FLT_MAX;
	for (size_t i = 0; i < p_hull->verts._length; ++i) {
		const float* p_vert = darr_get(&p_hull->verts, i);
		const float dot = vec3_dot(p_vert, p_dir);
		if (dot > dot_max) {
			dot_max = dot;
			vec3_set(p_vert, p_extreme);
		}
	}
}

void gjk_find_support(const struct physics_collider* p_a, const struct physics_collider* p_b, const float* p_dir, float* p_support) {
	float tmp[3];

	vec3_inv(p_dir, tmp);
	gjk_find_extreme(p_b, tmp, p_support);

	gjk_find_extreme(p_a, p_dir, tmp);

	//CX_DBG_LOG_FMT("gjk", "collider extremes: dir=[%f, %f, %f], a=[%f, %f, %f], b=[%f, %f, %f]\n", p_dir[0], p_dir[1], p_dir[2], tmp[0], tmp[1], tmp[2], p_support[0], p_support[1], p_support[2]);

	vec3_sub(tmp, p_support, p_support);
}

int gjk_process_simplex(float simplex[4][3], int* p_simplex_d, float* p_dir) {
	if (*p_simplex_d == 2) {
		gjk_process_simplex_line(simplex, p_simplex_d, p_dir);
		return 0;
	}

	if (*p_simplex_d == 3) {
		gjk_process_simplex_triangle(simplex, p_simplex_d, p_dir);
		return 0;
	}

	return gjk_process_simplex_tetrahedron(simplex, p_simplex_d, p_dir);
}

void gjk_process_simplex_line(float simplex[4][3], int* p_simplex_d, float* p_dir) {
	const float* p_a = simplex[1];
	const float* p_b = simplex[0];

	float ao[3];
	float ab[3];
	
	vec3_inv(p_a, ao);
	vec3_sub(p_b, p_a, ab);

	if (GJK_SAME_SIDE(ab, ao)) {
		// simplex is LINE A->B
		// direction is AB cross AO cross AB

		vec3_cross(ab, ao, p_dir);
		vec3_cross(p_dir, ab, p_dir);

		//CX_DBG_LOG_FMT("gjk", "simplex=line, origin contained in bounds, dir=[%f, %f, %f]\n", p_dir[0], p_dir[1], p_dir[2]);
	} else {
		// simplex is POINT A
		// direction is AO

		vec3_set(p_a, simplex[0]);
		*p_simplex_d = 1;

		vec3_set(ao, p_dir);

		//CX_DBG_LOG_FMT("gjk", "simplex=line, origin not in bounds, dir=[%f, %f, %f]\n", p_dir[0], p_dir[1], p_dir[2]);
	}
}

void gjk_process_simplex_triangle(float simplex[4][3], int* p_simplex_d, float* p_dir) {
	const float* p_a = simplex[2];
	const float* p_b = simplex[1];
	const float* p_c = simplex[0];

	float ao[3];
	float ab[3];
	float ac[3];
	float abc[3];

	vec3_inv(p_a, ao);
	vec3_sub(p_b, p_a, ab);
	vec3_sub(p_c, p_a, ac);
	vec3_cross(ab, ac, abc);

	float tmp[3];

	vec3_cross(abc, ac, tmp);
	if (GJK_SAME_SIDE(tmp, ao)) {
		if (GJK_SAME_SIDE(ac, ao)) {
			// simplex is LINE A->C
			// direction is AC cross AO cross AC

			vec3_set(p_c, simplex[0]);
			vec3_set(p_a, simplex[1]);
			*p_simplex_d = 2;

			vec3_cross(ac, ao, p_dir);
			vec3_cross(p_dir, ac, p_dir);
		} else {
			if (GJK_SAME_SIDE(ab, ao)) {
				// simplex is LINE A->B
				// direction is AB cross AO cross AB

				vec3_set(p_b, simplex[0]);
				vec3_set(p_a, simplex[1]);
				*p_simplex_d = 2;

				vec3_cross(ab, ao, p_dir);
				vec3_cross(p_dir, ab, p_dir);
			} else {
				// simplex is POINT A
				// direction is AO

				vec3_set(p_a, simplex[0]);
				*p_simplex_d = 1;

				vec3_set(p_dir, ao);
			}
		}
	} else {
		vec3_cross(ab, abc, tmp);
		if (GJK_SAME_SIDE(tmp, ao)) {
			if (GJK_SAME_SIDE(ab, ao)) {
				// simplex is LINE A->B
				// direction is AB cross AO cross AB

				vec3_set(p_b, simplex[0]);
				vec3_set(p_a, simplex[1]);
				*p_simplex_d = 2;

				vec3_cross(ab, ao, p_dir);
				vec3_cross(p_dir, ab, p_dir);
			} else {
				// simplex is POINT A
				// direction is AO

				vec3_set(p_a, simplex[0]);
				*p_simplex_d = 1;

				vec3_set(ao, p_dir);
			}
		} else {
			if (GJK_SAME_SIDE(abc, ao)) {
				// simpelx is TRIANGLE A->B->C
				// direction is abc

				vec3_set(abc, p_dir);
			} else {
				// simplex is TRIANGLE A->C->B
				// direction is -acb

				vec3_set(p_c, tmp);
				vec3_set(p_b, simplex[0]);
				vec3_set(tmp, simplex[1]);
				
				vec3_inv(abc, p_dir);
			}
		}
	}
}

int gjk_process_simplex_tetrahedron(float simplex[4][3], int* p_simplex_d, float* p_dir) {
	const float* p_a = simplex[3];
	const float* p_b = simplex[2];
	const float* p_c = simplex[1];
	const float* p_d = simplex[0];

	float ao[3];
	float ab[3];
	float ac[3];
	float ad[3];
	float cross[3];

	vec3_inv(p_a, ao);
	vec3_sub(p_b, p_a, ab);
	vec3_sub(p_c, p_a, ac);
	vec3_sub(p_d, p_a, ad);

	vec3_cross(ab, ac, cross);
	if (GJK_SAME_SIDE(cross, ao)) {
		// Simplex is TRANGLE A->B->C
		vec3_set(p_c, simplex[0]);
		vec3_set(p_b, simplex[1]);
		vec3_set(p_a, simplex[2]);
		*p_simplex_d = 3;
		gjk_process_simplex_triangle(simplex, p_simplex_d, p_dir);
		return 0;
	}

	vec3_cross(ac, ad, cross);
	if (GJK_SAME_SIDE(cross, ao)) {
		// Simplex is TRANGLE A->C->D
		vec3_set(p_a, simplex[2]);
		*p_simplex_d = 3;
		gjk_process_simplex_triangle(simplex, p_simplex_d, p_dir);
		return 0;
	}

	vec3_cross(ad, ab, cross);
	if (GJK_SAME_SIDE(cross, ao)) {
		// Simplex is TRANGLE A->B->D
		vec3_set(p_b, simplex[1]);
		vec3_set(p_a, simplex[2]);
		*p_simplex_d = 3;
		gjk_process_simplex_triangle(simplex, p_simplex_d, p_dir);
		return 0;
	}

	return 1;
}

#undef GJK_SAME_SIDE

void epa(float simplex[4][3], const struct physics_collider* p_a, const struct physics_collider* p_b, struct physics_collision_result* p_result) {
	// todo: use epa algorith to determine collision points, depth, normal
	*p_result = (struct physics_collision_result){0};
}