#ifndef CX_RENDER_PIPELINE_H
#define CX_RENDER_PIPELINE_H

#include <stdint.h>

#include "cx_asset_defs.h"

enum cx_depth_test_func {
	CX_DEPTH_TEST_FUNC_never,
	CX_DEPTH_TEST_FUNC_always,
	CX_DEPTH_TEST_FUNC_equal,
	CX_DEPTH_TEST_FUNC_not_equal,
	CX_DEPTH_TEST_FUNC_less,
	CX_DEPTH_TEST_FUNC_less_or_equal,
	CX_DEPTH_TEST_FUNC_greater,
	CX_DEPTH_TEST_FUNC_greater_or_equal,
};

enum cx_blend_func {
	CX_BLEND_FUNC_zero,
	CX_BLEND_FUNC_one,
	CX_BLEND_FUNC_src_color,
	CX_BLEND_FUNC_one_minus_src_color,
	CX_BLEND_FUNC_dst_color,
	CX_BLEND_FUNC_one_minus_dst_color,
	CX_BLEND_FUNC_src_alpha,
	CX_BLEND_FUNC_one_minus_src_alpha,
	CX_BLEND_FUNC_dst_alpha,
	CX_BLEND_FUNC_dst_minus_src_alpha,
	CX_BLEND_FUNC_blend_color,
	CX_BLEND_FUNC_one_minus_blend_color,
	CX_BLEND_FUNC_blend_color_alpha,
	CX_BLEND_FUNC_one_minus_blend_color_alpha
};

enum cx_cull_mode {
	CX_CULL_MODE_none,
	CX_CULL_MODE_back,
	CX_CULL_MODE_front,
	CX_CULL_MODE_front_and_back
};

enum cx_render_pipeline_flag {
	CX_RENDER_PIPELINE_FLAG_depth_test_enabled                    = 0x1,
	CX_RENDER_PIPELINE_FLAG_depth_writes_enabled                  = 0x2,
	CX_RENDER_PIPELINE_FLAG_blend_enabled                         = 0x4,
	CX_RENDER_PIPELINE_FLAG_front_face_clockwise_ordering_enabled = 0x8
};

struct cx_shader;

struct cx_render_pipeline {
	struct cx_asset_ref shader_asset_ref;

	uint8_t flags;

	enum cx_depth_test_func depth_test_func;

	enum cx_blend_func blend_src_func;
	enum cx_blend_func blend_dst_func;
	float              blend_color[4];

	enum cx_cull_mode cull_mode;
};

#endif
