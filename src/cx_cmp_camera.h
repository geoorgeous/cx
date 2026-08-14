#ifndef CX_CMP_CAMERA_H
#define CX_CMP_CAMERA_H

#include <stdint.h>

#include "cx_component.h"

extern struct cx_component_type cmp_type_camera;

enum cx_camera_projection_type {
	CX_CAMERA_PROJECTION_TYPE_perspective,
	CX_CAMERA_PROJECTION_TYPE_orthographic
};

struct cx_perspective_projection_settings {
	float vertical_fov;
	float aspect_ration;
	float near_plane;
	float far_plane;
};

struct cx_orthographic_projection_settings {
	float left;
	float right;
	float top;
	float bottom;
	float near;
	float far;
};

struct cx_cmp_camera {
	uint16_t priority;
	enum cx_camera_projection_type projection_type;
	union {
		struct cx_perspective_projection_settings persp;
		struct cx_orthographic_projection_settings ortho;
	} projection_settings;
	float projection_matrix[16];
};

#endif
