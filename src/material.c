#include "cx_macro.h"
#include "cx_stream_serialization.h"
#include "material.h"

int material_serialize(const struct material* p_material, struct cx_stream_writer* p_writer) {
	cx_asset_serialize_handle(p_writer, p_material->p_texture);
	cx_stream_serialize_bytes(p_writer, sizeof(p_material->color), p_material->color);
	return CX_TRUE;
}

int material_deserialize(struct material* p_material, struct cx_stream_reader* p_reader) {
	cx_asset_deserialize_handle(p_reader, &p_material->p_texture);
	cx_stream_deserialize_bytes(p_reader, sizeof(p_material->color), p_material->color);
	return CX_TRUE;
}
