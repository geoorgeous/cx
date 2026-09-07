#include "cx_asset.h"
#include "cx_macro.h"
#include "cx_material.h"
#include "cx_stream_serialization.h"

int cx_material_serialize(const struct cx_material* p_material, struct cx_stream* p_stream) {
	cx_asset_ref_serialize(&p_material->render_pipeline.shader_asset_ref, p_stream);
	cx_stream_serialize_uint8(p_stream, p_material->render_pipeline.flags);
	
	if (p_material->render_pipeline.flags & CX_RENDER_PIPELINE_FLAG_depth_test_enabled) {
		cx_stream_serialize_uint8(p_stream, (uint8_t)p_material->render_pipeline.depth_test_func);
	}
	
	if (p_material->render_pipeline.flags & CX_RENDER_PIPELINE_FLAG_blend_enabled) {
		cx_stream_serialize_uint8(p_stream, (uint8_t)p_material->render_pipeline.blend_src_func);
		cx_stream_serialize_uint8(p_stream, (uint8_t)p_material->render_pipeline.blend_dst_func);
		cx_stream_serialize_bytes(
			p_stream, sizeof(p_material->render_pipeline.blend_color), p_material->render_pipeline.blend_color);
	}

	cx_stream_serialize_uint8(p_stream, (uint8_t)p_material->render_pipeline.cull_mode);

	cx_render_param_set_serialize(&p_material->param_set, p_stream);

	return CX_TRUE;
}

int cx_material_deserialize(struct cx_stream* p_stream, struct cx_material* p_material) {
	cx_asset_ref_deserialize(p_stream, &p_material->render_pipeline.shader_asset_ref);
	cx_stream_deserialize_uint8(p_stream, &p_material->render_pipeline.flags);

	uint8_t temp;

	if (p_material->render_pipeline.flags & CX_RENDER_PIPELINE_FLAG_depth_test_enabled) {
		cx_stream_deserialize_uint8(p_stream, &temp);
		p_material->render_pipeline.depth_test_func = temp;
	}

	if (p_material->render_pipeline.flags & CX_RENDER_PIPELINE_FLAG_blend_enabled) {
		cx_stream_deserialize_uint8(p_stream, &temp);
		p_material->render_pipeline.blend_src_func = temp;
		cx_stream_deserialize_uint8(p_stream, &temp);
		p_material->render_pipeline.blend_dst_func = temp;
		cx_stream_deserialize_bytes(
			p_stream, sizeof(p_material->render_pipeline.blend_color), p_material->render_pipeline.blend_color);
	}

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_material->render_pipeline.cull_mode = temp;

	cx_render_param_set_deserialize(p_stream, &p_material->param_set);

	return CX_TRUE;
}

void cx_material_asset_enumerate_dependencies(
	const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr) {

	const struct cx_material* p_material = p_asset;
	f_cb(p_material->render_pipeline.shader_asset_ref.asset_id, p_user_ptr);

	cx_render_param_set_enumerate_dependencies(&p_material->param_set, f_cb, p_user_ptr);
}
