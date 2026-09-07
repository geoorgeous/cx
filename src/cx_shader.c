#include "cx_shader.h"
#include "cx_stream.h"
#include "cx_stream_serialization.h"

int cx_shader_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	const struct cx_shader* p_shader = p_asset;
	cx_stream_serialize_string(p_stream, p_shader->source.p_vertex_source, p_shader->source.vertex_source_len);
	cx_stream_serialize_string(p_stream, p_shader->source.p_fragment_source, p_shader->source.fragment_source_len);
	return CX_TRUE;
}

int cx_shader_asset_deserialize(struct cx_stream* p_stream, void* p_asset) {
	struct cx_shader* p_shader = p_asset;
	// TODO(george):
	return CX_TRUE;
}
