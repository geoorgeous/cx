#include "matrix.h"
#include "transform.h"
#include "vector.h"

#include "logging.h"

void transform_make_identity(struct transform* p_transform) {
    *p_transform = (struct transform) {0};
    quaternion_identity(p_transform->rotation);
    vec3_set_s(1, p_transform->scale);
}

void transform_reset_local(struct transform* p_transform) {
    vec3_clr(p_transform->position);
    quaternion_identity(p_transform->rotation);
    vec3_set_s(1, p_transform->scale);
    
    if (p_transform->p_local_transform) {
        vec3_set(p_transform->p_local_transform->world_position, p_transform->world_position);
        vec_set(4, p_transform->p_local_transform->world_rotation, p_transform->world_rotation);
        vec3_set(p_transform->p_local_transform->world_scale, p_transform->world_scale);
    }
}

void transform_compute_world_trs_matrix(struct transform* p_transform) {    
    if (p_transform->p_local_transform) {
        transform_compute_world_trs_matrix(p_transform->p_local_transform);
        vec3_add(p_transform->p_local_transform->world_position, p_transform->position, p_transform->world_position);
        quaternion_multiply(p_transform->p_local_transform->world_rotation, p_transform->rotation, p_transform->world_rotation);
        vec3_mul(p_transform->p_local_transform->world_scale, p_transform->scale, p_transform->world_scale);
    } else {
        vec3_set(p_transform->position, p_transform->world_position);
        vec_set(4, p_transform->rotation, p_transform->world_rotation);
        vec3_set(p_transform->scale, p_transform->world_scale);
    }

    float temp[16];
    matrix_make_scale(p_transform->world_scale[0], p_transform->world_scale[1], p_transform->world_scale[2], temp);
    matrix_make_rotation_from_quaternion(p_transform->world_rotation, p_transform->world_trs_matrix);
    matrix_multiply(p_transform->world_trs_matrix, temp, p_transform->world_trs_matrix);
    matrix_make_translation(p_transform->world_position[0], p_transform->world_position[1], p_transform->world_position[2], temp);
    matrix_multiply(temp, p_transform->world_trs_matrix, p_transform->world_trs_matrix);
}

void transform_copy(const struct transform* p_transform, struct transform* p_result) {
    vec3_set(p_transform->position, p_result->position);
    vec_set(4, p_transform->rotation, p_result->rotation);
    vec3_set(p_transform->scale, p_result->scale);
    vec3_set(p_transform->world_position, p_result->world_position);
    vec_set(4, p_transform->world_rotation, p_result->world_rotation);
    vec3_set(p_transform->world_scale, p_result->world_scale);
    matrix_copy(p_transform->world_trs_matrix, p_result->world_trs_matrix);
    p_result->p_local_transform = p_transform->p_local_transform;
}

void transform_set_world_position(struct transform* p_transform, const float* p_position) {
    if (p_transform->p_local_transform) {
        vec3_sub(p_position, p_transform->p_local_transform->world_position, p_transform->position);
    } else {
        vec3_set(p_position, p_transform->position);
    }
    vec3_set(p_position, p_transform->world_position);
}

void transform_set_world_rotation(struct transform* p_transform, const float* p_rotation) {
    if (p_transform->p_local_transform) {
        quaternion_conjugate(p_transform->p_local_transform->world_rotation, p_transform->rotation);
        quaternion_multiply(p_transform->rotation, p_rotation, p_transform->rotation);
    } else {
        vec3_set(p_rotation, p_transform->rotation);
    }
    vec3_set(p_rotation, p_transform->world_rotation);
}

void transform_set_world_scale(struct transform* p_transform, const float* p_scale) {
    if (p_transform->p_local_transform) {
        vec3_div(p_scale, p_transform->p_local_transform->world_scale, p_transform->scale);
    } else {
        vec3_set(p_scale, p_transform->scale);
    }
    vec3_set(p_scale, p_transform->world_scale);
}

int transform_set_local_transform(struct transform* p_transform, struct transform* p_local_transform, int b_preserve_world_transform) {
    struct transform* p_t = p_local_transform;
    while (p_t) {
        if (p_t == p_transform) {
            return 0;
        }
        p_t = p_t->p_local_transform;
    }

    p_transform->p_local_transform = p_local_transform;
    transform_set_world_position(p_transform, p_transform->world_position);
    transform_set_world_rotation(p_transform, p_transform->world_rotation);
    transform_set_world_scale(p_transform, p_transform->world_scale);

    return 1;
}