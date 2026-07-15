#include "cx_macro.h"
#include "cx_stream_serialization.h"
#include "material.h"

int material_serialize(const struct material* p_material, struct cx_stream* p_stream) {
	cx_asset_handle_serialize(p_material->p_texture, p_stream);
	cx_stream_serialize_bytes(p_stream, sizeof(p_material->color), p_material->color);
	return CX_TRUE;
}

int material_deserialize(struct cx_stream* p_stream, struct material* p_out_material) {
	cx_asset_handle_deserialize(p_stream, &p_out_material->p_texture);
	cx_stream_deserialize_bytes(p_stream, sizeof(p_out_material->color), p_out_material->color);
	return CX_TRUE;
}
