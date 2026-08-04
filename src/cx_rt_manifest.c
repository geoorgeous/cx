#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_asset_package.h"
#include "cx_macro.h"
#include "cx_rt_manifest.h"
#include "cx_stream_file.h"
#include "cx_stream_serialization.h"

static int cx_asset_source_deserialize_package_asset(cx_asset_id id, void* p_context, void* p_out) {
	const struct cx_asset_package* p_package = p_context;
	return cx_asset_package_deserialize_asset(p_package, id, p_out);
}

int cx_rt_manifest_serialize(const struct cx_rt_manifest* p_manifest, struct cx_stream* p_stream) {
	cx_stream_serialize_bytes(p_stream, sizeof(p_manifest->window_size), p_manifest->window_size);
	cx_stream_serialize_string(p_stream, p_manifest->title_buf, 0);

	cx_stream_serialize_uint16(p_stream, (uint16_t)p_manifest->asset_packages.length);
	for (uint16_t i = 0; i < p_manifest->asset_packages.length; ++i) {
		const struct cx_asset_package* p_package = cx_array_at(&p_manifest->asset_packages, i);
		cx_stream_serialize_string(p_stream, p_package->s_filename_, 0);
	}

	cx_asset_ref_serialize(&p_manifest->start_world_blueprint_ref, p_stream);

	return CX_TRUE;
}

int cx_rt_manifest_deserialize(struct cx_stream* p_stream, struct cx_rt_manifest* p_out_manifest) {
	*p_out_manifest = (struct cx_rt_manifest){0};
	
	cx_stream_deserialize_bytes(p_stream, sizeof(p_out_manifest->window_size), p_out_manifest->window_size);

	size_t temp_len;

	cx_stream_deserialize_cstring(p_stream, p_out_manifest->title_buf, &temp_len);

	char asset_package_filename_buf[256];

	uint16_t num_asset_packages;
	cx_stream_deserialize_uint16(p_stream, &num_asset_packages);

	for (uint16_t i = 0; i < num_asset_packages; ++i) {
		cx_stream_deserialize_cstring(p_stream, asset_package_filename_buf, &temp_len);

		struct cx_asset_package* p_package = cx_array_push(&p_out_manifest->asset_packages, CX_NULL);
		cx_asset_package_import(asset_package_filename_buf, p_package);
		cx_asset_cache_push_source(&(struct cx_asset_source) {
			.p_context = p_package,
			.f_try_deserialize_asset = cx_asset_source_deserialize_package_asset
		});
	}

	cx_asset_ref_deserialize(p_stream, &p_out_manifest->start_world_blueprint_ref);

	return CX_TRUE;
}

void cx_rt_manifest_save_to_file(const struct cx_rt_manifest* p_manifest, const char* s_filename) {
	struct cx_stream_file stream;
	cx_stream_file_open(s_filename, "wb", &stream);

	cx_rt_manifest_serialize(p_manifest, &stream.base);

	cx_stream_close(&stream.base);
}

int cx_rt_manifest_load_from_file(const char* s_filename, struct cx_rt_manifest* p_out_manifest) {
	struct cx_stream_file stream;
	if (!cx_stream_file_open(s_filename, "rb", &stream)) {
		return CX_FALSE;
	}

	cx_rt_manifest_deserialize(&stream.base, p_out_manifest);

	cx_stream_close(&stream.base);

	return CX_TRUE;
}
