#include "cx_macro.h"
#include "cx_rt_manifest.h"
#include "cx_stream_file.h"
#include "cx_stream_serialization.h"

int cx_rt_manifest_serialize(const struct cx_rt_manifest* p_manifest, struct cx_stream* p_stream) {
	cx_stream_serialize_bytes(p_stream, sizeof(p_manifest->window_size), p_manifest->window_size);
	cx_stream_serialize_string(p_stream, p_manifest->title_buf, 0);

	cx_stream_serialize_uint16(p_stream, p_manifest->num_asset_packages);
	for (uint16_t i = 0; i < p_manifest->num_asset_packages; ++i) {
		cx_stream_serialize_string(p_stream, p_manifest->p_asset_packages[i].s_filename_, 0);
	}

	cx_asset_handle_serialize(p_manifest->p_start_world_blueprint, p_stream);

	return CX_TRUE;
}

int cx_rt_manifest_deserialize(struct cx_stream* p_stream, struct cx_rt_manifest* p_out_manifest) {
	*p_out_manifest = (struct cx_rt_manifest){0};
	
	cx_stream_deserialize_bytes(p_stream, sizeof(p_out_manifest->window_size), p_out_manifest->window_size);

	size_t temp_len;

	cx_stream_deserialize_string(p_stream, p_out_manifest->title_buf, &temp_len);
	p_out_manifest->title_buf[temp_len] = '\0';

	char asset_package_filename_buf[256];

	cx_stream_deserialize_uint16(p_stream, &p_out_manifest->num_asset_packages);
	for (uint16_t i = 0; i < p_out_manifest->num_asset_packages; ++i) {
		cx_stream_deserialize_string(p_stream, asset_package_filename_buf, &temp_len);
		asset_package_filename_buf[temp_len] = '\0';

		cx_asset_package_load_records_from_file(&p_out_manifest->p_asset_packages[i], asset_package_filename_buf);
		cx_asset_directory_register_package(&p_out_manifest->p_asset_packages[i]);
	}

	cx_asset_handle_deserialize(p_stream, &p_out_manifest->p_start_world_blueprint);

	return CX_TRUE;
}

void cx_rt_manifest_save_to_file(const struct cx_rt_manifest* p_manifest, const char* s_filename) {
	struct cx_stream_file stream;
	cx_stream_file_open(s_filename, "wb", &stream);

	cx_rt_manifest_serialize(p_manifest, &stream.base);

	cx_stream_file_close(&stream);
}

int cx_rt_manifest_load_from_file(const char* s_filename, struct cx_rt_manifest* p_out_manifest) {
	struct cx_stream_file stream;
	if (!cx_stream_file_open(s_filename, "rb", &stream)) {
		return CX_FALSE;
	}

	cx_rt_manifest_deserialize(&stream.base, p_out_manifest);

	cx_stream_file_close(&stream);

	return CX_TRUE;
}
