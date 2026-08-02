#include "cx_asset.h"
#include "cx_ed_asset_package_builder.h"
#include "cx_stream_file.h"
#include "cx_stream_serialization.h"

void cx_ed_asset_package_builder_add_asset(
	struct cx_ed_asset_package_builder* p_builder, const struct cx_asset_ref* p_asset_ref) {
	
	if (p_builder->assets.element_size == 0) {
		cx_array_init(sizeof(struct cx_asset_ref), &p_builder->assets);
	}

	(void)cx_array_push(&p_builder->assets, p_asset_ref);
}

void cx_ed_asset_package_builder_export(const struct cx_ed_asset_package_builder* p_builder, const char* s_filepath) {
	struct cx_stream_file stream;

	cx_stream_file_open(s_filepath, "wb", &stream);

	cx_stream_serialize_uint32(&stream.base, (uint32_t)p_builder->assets.length);

	for (cx_asset_type i = 0; i < p_builder->assets.length; ++i) {
		const struct cx_asset_ref* p_asset = cx_array_at(&p_builder->assets, i);

		cx_stream_serialize_uint32(&stream.base, p_asset->asset_id);
		cx_stream_serialize_uint32(&stream.base, 0);
	}

	const size_t package_record_size = sizeof(uint32_t) * 2;

	size_t asset_data_offset = cx_stream_tell(&stream.base);

	for (cx_asset_type i = 0; i < p_builder->assets.length; ++i) {
		struct cx_asset_ref* p_asset = cx_array_at(&p_builder->assets, i);

		CX_LOG_FMT(TRACE, ASSET, "  Saving asset %x...\n", p_asset->asset_id);
		const int b_result = cx_asset_type_serialize_asset(i, *p_asset->pp_asset, &stream.base);

		if (!b_result) {
			// todo: handle serialization error
			CX_LOG_FMT(ERROR, ASSET, "Asset serialization error: asset_id=%x\n", p_asset->asset_id);
		}

		// Cache the next record's data location
		const size_t next_asset_data_offset = cx_stream_tell(&stream.base);

		// write the asset's file location now that we know it
		cx_stream_seek(&stream.base, sizeof(uint32_t) + package_record_size * i, CX_STREAM_SEEK_ORIGIN_begin);

		cx_stream_serialize_uint32(&stream.base, (uint32_t)asset_data_offset);

		cx_stream_seek(&stream.base, (ptrdiff_t)next_asset_data_offset, CX_STREAM_SEEK_ORIGIN_begin);
	}

	cx_stream_close(&stream.base);
}

void cx_ed_asset_package_builder_free(struct cx_ed_asset_package_builder* p_builder) {
	cx_array_free(&p_builder->assets);
}
