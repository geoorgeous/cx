#ifndef _H__TRANSFORM
#define _H__TRANSFORM

struct transform {
    float             position[3];
    float             rotation[4];
    float             scale[3];
    float             world_position[3];
    float             world_rotation[4];
    float             world_scale[3];
    float             world_trs_matrix[16];
    struct transform* p_local_transform;
};

void transform_make_identity(struct transform* p_transform);
void transform_reset_local(struct transform* p_transform);
void transform_compute_world_trs_matrix(struct transform* p_transform);
void transform_copy(const struct transform* p_transform, struct transform* p_result);
void transform_set_world_position(struct transform* p_transform, const float* p_position);
void transform_set_world_rotation(struct transform* p_transform, const float* p_rotation);
void transform_set_world_scale(struct transform* p_transform, const float* p_scale);
int  transform_set_local_transform(struct transform* p_transform, struct transform* p_local_transform, int b_preserve_world_transform);

#endif