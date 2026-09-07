#ifndef CX_MATERIAL_H
#define CX_MATERIAL_H

#include "cx_asset_defs.h"
#include "cx_render_parameter.h"
#include "cx_render_pipeline.h"

struct cx_stream;

struct cx_material {
	struct cx_render_pipeline render_pipeline;
	struct cx_render_param_set param_set;
};

int cx_material_serialize(const struct cx_material* p_material, struct cx_stream* p_stream);
int cx_material_deserialize(struct cx_stream* p_stream, struct cx_material* p_material);

static inline int cx_material_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return cx_material_serialize(p_asset, p_stream);
}

static inline int cx_material_asset_deserialize(struct cx_stream* p_stream, void* p_asset) {
	return cx_material_deserialize(p_stream, p_asset);
}

void cx_material_asset_enumerate_dependencies(
	const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr);

#endif
