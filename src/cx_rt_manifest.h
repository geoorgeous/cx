#ifndef CX_RT_MANIFEST_H
#define CX_RT_MANIFEST_H

#include <stdint.h>

#include "cx_asset.h"

struct cx_rt_manifest {
	char title_buf[64];
	uint32_t window_size[2];
	struct cx_asset_package* p_asset_packages;
	uint16_t num_asset_packages;
	cx_asset_handle p_start_world_blueprint;
};

struct cx_stream;

int cx_rt_manifest_serialize(const struct cx_rt_manifest* p_manifest, struct cx_stream* p_stream);
int cx_rt_manifest_deserialize(struct cx_stream* p_stream, struct cx_rt_manifest* p_out_manifest);
void cx_rt_manifest_save_to_file(const struct cx_rt_manifest* p_manifest, const char* s_filename);
int cx_rt_manifest_load_from_file(const char* s_filename, struct cx_rt_manifest* p_out_manifest);

#endif
