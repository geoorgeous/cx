#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_ed_asset_package_builder.h"
#include "cx_stream_file.h"
#include "cx_stream_serialization.h"

struct cx_ed_asset_package_builder_entry {
	struct cx_asset_ref asset_ref;
	uint32_t asset_name_file_off;
	uint32_t asset_data_file_off;
};

static void cx_ed_asset_package_builder_add_asset_internal(
	struct cx_ed_asset_package_builder* p_builder, const struct cx_asset_ref* p_asset_ref);

static void cx_asset_enumerate_dependencies_cb(cx_asset_id asset_id, void* p_user_ptr);

void cx_ed_asset_package_builder_add_asset(
	struct cx_ed_asset_package_builder* p_builder, const struct cx_asset_ref* p_asset_ref) {
	
	cx_asset_type_enumerate_dependencies(
		CX_ASSET_GET_TYPE_ID(p_asset_ref->asset_id),
		*p_asset_ref->pp_asset,
		cx_asset_enumerate_dependencies_cb,
		p_builder);

	cx_ed_asset_package_builder_add_asset_internal(p_builder, p_asset_ref);
}

void cx_ed_asset_package_builder_export(const struct cx_ed_asset_package_builder* p_builder, const char* s_filepath) {
	struct cx_stream_file stream;

	cx_stream_file_open(s_filepath, "wb", &stream);

	// Asset name string table

	const size_t asset_name_table_off = sizeof(uint32_t) + sizeof(uint32_t) * 3 * p_builder->entries.length;

	cx_stream_seek(&stream.base, (ptrdiff_t)asset_name_table_off, CX_STREAM_SEEK_ORIGIN_begin);

	for (size_t i = 0; i < p_builder->entries.length; ++i) {
		struct cx_ed_asset_package_builder_entry* p_entry = cx_array_at(&p_builder->entries, i);

		p_entry->asset_name_file_off = (uint32_t)cx_stream_tell(&stream.base);

		const char* s_asset_name;
		cx_asset_cache_get_name(p_entry->asset_ref.asset_id, &s_asset_name);
		cx_stream_serialize_string(&stream.base, s_asset_name, 0);
	}

	// Asset data blobs

	for (size_t i = 0; i < p_builder->entries.length; ++i) {
		struct cx_ed_asset_package_builder_entry* p_entry = cx_array_at(&p_builder->entries, i);

		p_entry->asset_data_file_off = (uint32_t)cx_stream_tell(&stream.base);

		const void* p_asset = cx_asset_cache_acquire(&p_entry->asset_ref);

		const int b_result = cx_asset_type_serialize_asset(
			CX_ASSET_GET_TYPE_ID(p_entry->asset_ref.asset_id),
			p_asset,
			&stream.base);

		if (!b_result) {
			// todo: handle serialization error
			CX_LOG_FMT(ERROR, ASSET_PACKAGE_BUILDER, "Asset serialization error: asset_id=%X\n", &p_entry->asset_ref);
		}
	}

	// Package record table

	cx_stream_seek(&stream.base, 0, CX_STREAM_SEEK_ORIGIN_begin);

	cx_stream_serialize_uint32(&stream.base, (uint32_t)p_builder->entries.length);

	for (size_t i = 0; i < p_builder->entries.length; ++i) {
		struct cx_ed_asset_package_builder_entry* p_entry = cx_array_at(&p_builder->entries, i);

		const char* s_asset_name;
		cx_asset_cache_get_name(p_entry->asset_ref.asset_id, &s_asset_name);

		CX_LOG_FMT(INFO, ASSET_PACKAGE_BUILDER, "  [%u] %s (%s:%X) name_off=%u, data_off=%u\n",
			i,
			s_asset_name,
			cx_asset_type_display_name_str(CX_ASSET_GET_TYPE_ID(p_entry->asset_ref.asset_id)),
			p_entry->asset_ref.asset_id,
			p_entry->asset_name_file_off,
			p_entry->asset_data_file_off);

		cx_stream_serialize_uint32(&stream.base, p_entry->asset_ref.asset_id);
		cx_stream_serialize_uint32(&stream.base, p_entry->asset_name_file_off);
		cx_stream_serialize_uint32(&stream.base, p_entry->asset_data_file_off);

		cx_asset_cache_release(&p_entry->asset_ref);
	}

	cx_stream_close(&stream.base);
}

void cx_ed_asset_package_builder_free(struct cx_ed_asset_package_builder* p_builder) {
	cx_array_free(&p_builder->entries);
}

void cx_ed_asset_package_builder_add_asset_internal(
	struct cx_ed_asset_package_builder* p_builder, const struct cx_asset_ref* p_asset_ref) {

	if (p_builder->entries.element_size == 0) {
		cx_array_init(sizeof(struct cx_ed_asset_package_builder_entry), &p_builder->entries);
	}

	(void)cx_array_push(&p_builder->entries, &(struct cx_ed_asset_package_builder_entry) {
		.asset_ref = { .asset_id = p_asset_ref->asset_id }
	});
}

void cx_asset_enumerate_dependencies_cb(cx_asset_id asset_id, void* p_user_ptr) {
	struct cx_ed_asset_package_builder* p_builder = p_user_ptr;
	
	cx_ed_asset_package_builder_add_asset_internal(p_builder, &(struct cx_asset_ref) {
		.asset_id = asset_id	
	});
}
