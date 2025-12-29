#include <stdlib.h>
#include <string.h>

#include "skeletal_animation.h"
#include "skeleton.h"
#include "transform_animation.h"

float skeletal_animation_duration(const struct skeletal_animation* p_skeletal_animation) {
	float duration = 0;
	for (size_t i = 0; i < p_skeletal_animation->num_joint_transform_animations; ++i) {
		const float joint_transform_animation_duration = transform_animation_sampler_duration(&p_skeletal_animation->p_joint_transform_animations[i].sampler);
		if (joint_transform_animation_duration > duration) {
			duration = joint_transform_animation_duration;
		}
	}
	return duration;
}