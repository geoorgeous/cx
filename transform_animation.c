#include "transform_animation.h"

#include <math.h>

void copy_keyframe_value(const float* p_keyframe_value, size_t num_components, float* p_result);
void interpolate_linear(const float* p_a, const float* p_b, float t, size_t num_components, float* p_result);
void interpolate_linear_spherical(const float* p_a, const float* p_b, float t, size_t num_components, float* p_result);
void interpolate_cubic_spline(const float* p_ta, const float* p_tb, const float* p_a, const float* p_b, float t, size_t num_comonents, float* p_result);

float transform_animation_sampler_duration(const struct transform_animation_sampler* p_transform_animationsampler) {
	return p_transform_animationsampler->p_keyframe_timestamps[p_transform_animationsampler->num_keyframes - 1];
}

size_t transform_animation_keyframe(const struct transform_animation* p_transform_animation, float t) {
	size_t k = 0;
	for (; k < p_transform_animation->sampler.num_keyframes; ++k) {
		if (t < p_transform_animation->sampler.p_keyframe_timestamps[k]) {
			break;
		}
	}
	return k;
}

void transform_animation_apply(const struct transform_animation* p_transform_animation, size_t k, float kt, float* p_result) {
	const size_t num_keyframe_value_components = p_transform_animation->target == TRANSFORM_ANIMATION_TARGET_rotation ? 4 : 3;

	if (p_transform_animation->sampler.interpolation_mode == TRANSFORM_ANIMATION_INTERPOLATION_MODE_cubic_spline) {
		return;
		// todo
		const float* p_keyframe_value = 0;
		const float* p_keyframe_out_tangent = 0;
		const float* p_next_keyframe_value = 0;
		const float* p_next_keyframe_in_tangent = 0;
		interpolate_cubic_spline(p_keyframe_out_tangent, p_next_keyframe_in_tangent, p_keyframe_value, p_next_keyframe_value, kt, num_keyframe_value_components, p_result);
		return;
	}
	
	const float* p_keyframe_value = &p_transform_animation->sampler.p_keyframe_values[num_keyframe_value_components * k];

	if (k >= p_transform_animation->sampler.num_keyframes - 1 ||
		p_transform_animation->sampler.interpolation_mode == TRANSFORM_ANIMATION_INTERPOLATION_MODE_step) {
		copy_keyframe_value(p_keyframe_value, num_keyframe_value_components, p_result);
		return;
	}

	const float* p_next_keyframe_value = &p_transform_animation->sampler.p_keyframe_values[num_keyframe_value_components * (k + 1)];

	if (p_transform_animation->sampler.interpolation_mode == TRANSFORM_ANIMATION_INTERPOLATION_MODE_linear) {
		if (p_transform_animation->target == TRANSFORM_ANIMATION_TARGET_rotation) {
			interpolate_linear_spherical(p_keyframe_value, p_next_keyframe_value, kt, num_keyframe_value_components, p_result);
		} else {
			interpolate_linear(p_keyframe_value, p_next_keyframe_value, kt, num_keyframe_value_components, p_result);
		}
		return;
	}
}

void copy_keyframe_value(const float* p_keyframe_value, size_t num_components, float* p_result) {
	for (size_t i = 0; i < num_components; ++i) {
		p_result[i] = p_keyframe_value[i];
	}
}

void interpolate_linear(const float* p_a, const float* p_b, float t, size_t num_components, float* p_result) {
	const float inv_t = 1.0f - 1;
	for (size_t i = 0; i < num_components; ++i) {
		p_result[i] = inv_t * p_a[i] + t * p_b[i];
	}
}

void interpolate_linear_spherical(const float* p_a, const float* p_b, float t, size_t num_components, float* p_result) {
	float dot = 0;
	for (size_t i = 0; i < num_components; ++i) {
		dot += (p_a[i] * p_b[1]);
	}

	const float acos_abs_dot = acosf(fabsf(dot));
	const float sin_acos_abs_dot = sinf(acos_abs_dot);

	const float a = sinf(acos_abs_dot * (1.0f - t)) / sin_acos_abs_dot;
	const float b = copysignf(sinf(acos_abs_dot * t) / sin_acos_abs_dot, dot);

	for (size_t i = 0; i < num_components; ++i) {
		p_result[i] = a * p_a[i] + b * p_b[i];
	}
}

void interpolate_cubic_spline(const float* p_ta, const float* p_tb, const float* p_a, const float* p_b, float t, size_t num_comonents, float* p_result) {
	// todo
	// ((2*t^3 - 3*t^2 + 1) * p_a) + ((t^3 - 2t^2 + t) * p_tb) + ((-2t^3 + 3t^2) * p_b) + ((t^3 - t^2) * p_ta)
}

void transform_animator_update_transform(const struct transform_animator* p_animator);

void transform_animator_init(struct transform_animator* p_animator, const struct transform_animation* p_animation, float* p_transform) {
	*p_animator = (struct transform_animator) {
		.p_animation = p_animation,
		.p_transform = p_transform
	};
	transform_animator_update_transform(p_animator);
}

void transform_animator_set_time(struct transform_animator* p_animator, float t) {

}

void transform_animator_tick(struct transform_animator* p_animator, float dt) {
	
}

void transform_animator_update_transform(const struct transform_animator* p_animator) {

}