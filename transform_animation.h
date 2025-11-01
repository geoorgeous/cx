#ifndef _H__TRANSFORM_ANIMATION
#define _H__TRANSFORM_ANIMATION

#include <stdint.h>

enum transform_animation_interpolation_mode {
    TRANSFORM_ANIMATION_INTERPOLATION_MODE_step,
    TRANSFORM_ANIMATION_INTERPOLATION_MODE_linear,
    TRANSFORM_ANIMATION_INTERPOLATION_MODE_cubic_spline
};

struct transform_animation_sampler {
    float*                                      p_keyframe_timestamps;
    float*                                      p_keyframe_values;
    size_t                                      num_keyframes;
    enum transform_animation_interpolation_mode interpolation_mode;
};

enum transform_animation_target {
    TRANSFORM_ANIMATION_TARGET_translation,
    TRANSFORM_ANIMATION_TARGET_rotation,
    TRANSFORM_ANIMATION_TARGET_scale
};

struct transform_animation {
    struct transform_animation_sampler sampler;
    enum transform_animation_target    target;
};

struct transform_animator {
    const struct transform_animation* p_animation;
    float                             keyframe_index;
    float                             keyframe_elapsed;
    float*                            p_transform;
};

float  transform_animation_sampler_duration(const struct transform_animation_sampler* p_sampler);
size_t transform_animation_keyframe(const struct transform_animation* p_transform_animation, float t);
void   transform_animation_apply(const struct transform_animation* p_transform_animation, size_t k, float kt, float* p_result);

void transform_animator_init(struct transform_animator* p_animator, const struct transform_animation* p_animation, float* p_transform);
void transform_animator_set_time(struct transform_animator* p_animator, float t);
void transform_animator_tick(struct transform_animator* p_animator, float dt);

#endif