#ifndef CX_SHADER_H
#define CX_SHADER_H

#include <stddef.h>
#include <stdint.h>

#include "cx_gfx_shader_program.h"
#include "cx_gfx_shader_program_interface.h"

#define CX_ASSET_TYPE_SHADER 9

struct cx_shader_source {
	const char* p_vertex_stage_source;
	size_t vertex_stage_source_len;
	const char* p_fragment_stage_source;
	size_t fragment_stage_source_len;
};

struct cx_shader {
	void* p_vertex_stage_source_buf;
	void* p_fragment_stage_source_buf;
	struct cx_shader_source source_;
	struct cx_gfx_shader_program gfx_program_;
	struct cx_gfx_shader_program_interface gfx_program_interface_;
	int b_is_gfx_program_outdated;
};

void cx_shader_free(struct cx_shader* p_shader);
void cx_shader_set_source(struct cx_shader* p_shader, const struct cx_shader_source* p_source);
void cx_shader_load_device_program(struct cx_shader* p_shader);
void cx_shader_unload_device_program(struct cx_shader* p_shader);

struct cx_stream;

int cx_shader_asset_serialize(const void* p_asset, struct cx_stream* p_stream);
int cx_shader_asset_deserialize(struct cx_stream* p_stream, void* p_asset);

static inline void cx_shader_asset_free(void* p_asset) {
	cx_shader_unload_device_program(p_asset);
}

#endif
