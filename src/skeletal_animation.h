#ifndef _H__SKELETAL_ANIMATION
#define _H__SKELETAL_ANIMATION

#include <stdint.h>

struct skeleton_instance;
struct transform_animation;
struct transform_animator;

struct skeletal_animation {
    struct transform_animation* p_joint_transform_animations;
    size_t*                     p_target_joint_indices;
    size_t                      num_joint_transform_animations;
};

struct skeletal_animator {
    struct skeleton_instance*        p_skeleton_instance;
    const struct skeletal_animation* p_skeletal_animation;
    struct transform_animator*       p_transform_animators;
};

float skeletal_animation_duration(const struct skeletal_animation* p_skeletal_animation);

#endif