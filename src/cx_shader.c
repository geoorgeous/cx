#include <string.h>

#include "cx_alloc.h"
#include "cx_gfx_shader_program.h"
#include "cx_shader.h"
#include "cx_stream.h"
#include "cx_stream_serialization.h"

static inline void cx_shader_free_source_bufs(struct cx_shader* p_shader);

void cx_shader_free(struct cx_shader* p_shader) {
	cx_shader_free_source_bufs(p_shader);
	cx_shader_unload_device_program(p_shader);
}

void cx_shader_set_source(struct cx_shader* p_shader, const struct cx_shader_source* p_source) {
	cx_shader_free_source_bufs(p_shader);

	p_shader->p_vertex_stage_source_buf = CX_MALLOC(p_source->vertex_stage_source_len);
	memcpy(p_shader->p_vertex_stage_source_buf, p_source->p_vertex_stage_source, p_source->vertex_stage_source_len);
	p_shader->source_.p_vertex_stage_source = p_shader->p_vertex_stage_source_buf;
	p_shader->source_.vertex_stage_source_len = p_source->vertex_stage_source_len;

	p_shader->p_fragment_stage_source_buf = CX_MALLOC(p_source->fragment_stage_source_len);
	memcpy(p_shader->p_fragment_stage_source_buf, p_source->p_fragment_stage_source, p_source->fragment_stage_source_len);
	p_shader->source_.p_fragment_stage_source = p_shader->p_fragment_stage_source_buf;
	p_shader->source_.fragment_stage_source_len = p_source->fragment_stage_source_len;

	p_shader->b_is_gfx_program_outdated = CX_TRUE;
}

void cx_shader_load_device_program(struct cx_shader* p_shader) {
	if (p_shader->b_is_gfx_program_outdated == CX_FALSE) {
		return;
	}

	if (p_shader->source_.vertex_stage_source_len == 0 ||
		p_shader->source_.fragment_stage_source_len == 0) {

		return;
	}

	cx_shader_unload_device_program(p_shader);
	cx_gfx_shader_program_create(&p_shader->gfx_program_);
	
	if (cx_gfx_shader_program_build_from_source(&p_shader->gfx_program_, &p_shader->source_) != CX_SUCCESS) {
		cx_gfx_shader_program_destroy(&p_shader->gfx_program_);
		return;
	}
	
	cx_gfx_shader_program_reflect_interface(&p_shader->gfx_program_, &p_shader->gfx_program_interface_);

	p_shader->b_is_gfx_program_outdated = CX_FALSE;
}

void cx_shader_unload_device_program(struct cx_shader* p_shader) {
	cx_gfx_shader_program_destroy(&p_shader->gfx_program_);

	p_shader->gfx_program_ = (struct cx_gfx_shader_program){0};
	p_shader->gfx_program_interface_ = (struct cx_gfx_shader_program_interface){0};
}

int cx_shader_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	const struct cx_shader* p_shader = p_asset;

	cx_stream_serialize_uint64(p_stream, p_shader->source_.vertex_stage_source_len);
	cx_stream_serialize_bytes(
		p_stream, p_shader->source_.vertex_stage_source_len, p_shader->source_.p_vertex_stage_source);

	cx_stream_serialize_uint64(p_stream, p_shader->source_.fragment_stage_source_len);
	cx_stream_serialize_bytes(
		p_stream, p_shader->source_.fragment_stage_source_len, p_shader->source_.p_fragment_stage_source);

	return CX_TRUE;
}

int cx_shader_asset_deserialize(struct cx_stream* p_stream, void* p_asset) {
	struct cx_shader* p_shader = p_asset;

	size_t len;
	void* p_buf;

	cx_stream_deserialize_uint64(p_stream, &len);
	if (len > 0) {
		p_buf = CX_MALLOC(len);
		cx_stream_deserialize_bytes(p_stream, len, p_buf);

		p_shader->source_.vertex_stage_source_len = len;
		p_shader->source_.p_vertex_stage_source = p_buf;
	}

	cx_stream_deserialize_uint64(p_stream, &len);
	if (len > 0) {
		p_buf = CX_MALLOC(len);
		cx_stream_deserialize_bytes(p_stream, len, p_buf);

		p_shader->source_.fragment_stage_source_len = len;
		p_shader->source_.p_fragment_stage_source = p_buf;
	}

	p_shader->b_is_gfx_program_outdated =
		p_shader->source_.vertex_stage_source_len > 0 ||
		p_shader->source_.fragment_stage_source_len > 0;

	return CX_TRUE;
}

void cx_shader_free_source_bufs(struct cx_shader* p_shader) {
	if (p_shader->p_vertex_stage_source_buf) {
		CX_FREE(p_shader->p_vertex_stage_source_buf);
		p_shader->source_.p_vertex_stage_source = CX_NULL;
		p_shader->source_.vertex_stage_source_len = 0;
	}

	if (p_shader->p_fragment_stage_source_buf) {
		CX_FREE(p_shader->p_fragment_stage_source_buf);
		p_shader->source_.p_fragment_stage_source = CX_NULL;
		p_shader->source_.fragment_stage_source_len = 0;
	}
}
