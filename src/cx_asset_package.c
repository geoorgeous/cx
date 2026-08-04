#include <string.h>

#include "cx_asset.h"
#include "cx_asset_package.h"
#include "cx_logging.h"
#include "cx_stream_file.h"
#include "cx_stream_serialization.h"

int cx_asset_package_import(const char* s_filename, struct cx_asset_package* p_out) {
	*p_out = (struct cx_asset_package){0};

	struct cx_stream_file stream;
	
	if (!cx_stream_file_open(s_filename, "rb", &stream)) {
		return CX_FALSE;
	}

	strcpy(p_out->s_filename_, s_filename);

	CX_LOG_FMT(INFO, ASSET_PACKAGE, "Reading asset records from file '%s'...\n", p_out->s_filename_);

	const int b_result = cx_asset_package_deserialize_records(p_out, &stream.base);

	cx_stream_close(&stream.base);

	return b_result;
}

void cx_asset_package_free(struct cx_asset_package* p_package) {
	CX_LOG_FMT(TRACE, ASSET_PACKAGE, "Freeing asset package (%s)\n", p_package->s_filename_);

	for (uint8_t i = 0; i < CX_ASSET_TYPE_ID_MAX; ++i) {
		hashtable_free(&p_package->asset_type_record_tables_[i]);
	}
}

int cx_asset_package_deserialize_records(struct cx_asset_package* p_package, struct cx_stream* p_stream) {
	uint32_t num_records = 0;
	cx_stream_deserialize_uint32(p_stream, &num_records);
	
	CX_LOG_FMT(INFO, ASSET_PACKAGE, "Deserializing %u asset records...\n", num_records);

	if (num_records == 0) {
		return CX_TRUE;
	}

	for (size_t i = 0; i < num_records; ++i) {
		cx_asset_id id;
		cx_stream_deserialize_uint32(p_stream, &id);

		struct hashtable* p_table = &p_package->asset_type_record_tables_[CX_ASSET_GET_TYPE_ID(id)];

		if (p_table->element_size_ == 0) {
			hashtable_init(p_table, sizeof(struct cx_asset_package_record));
		}

		struct cx_asset_package_record* p_new_record = hashtable_i_add(p_table, id);
		*p_new_record = (struct cx_asset_package_record) {
			.p_package_ = p_package
		};

		uint32_t asset_name_file_off;
		cx_stream_deserialize_uint32(p_stream, &asset_name_file_off);
		cx_stream_deserialize_uint32(p_stream, &p_new_record->file_location_);

		cx_stream_seek(p_stream, asset_name_file_off, CX_STREAM_SEEK_ORIGIN_begin);

		size_t asset_name_len;
		cx_stream_deserialize_cstring(p_stream, p_new_record->name, &asset_name_len);

		const size_t next_asset_record_off =
			sizeof(uint32_t) +   // Num records
			sizeof(uint32_t) * 3 // Size of record data
			* (i + 1);           // Next record index
		cx_stream_seek(p_stream, (ptrdiff_t)next_asset_record_off, CX_STREAM_SEEK_ORIGIN_begin);

		CX_LOG_FMT(INFO, ASSET_PACKAGE, "  type=%u(%s), id=%u, name=%s, file_offset=%u\n",
			CX_ASSET_GET_TYPE_ID(id),
			cx_asset_type_display_name_str(CX_ASSET_GET_TYPE_ID(id)),
			id,
			p_new_record->name,
			p_new_record->file_location_);
	}

	return CX_TRUE;
}

int cx_asset_package_find_record(
	const struct cx_asset_package* p_package,
	cx_asset_id id,
	const struct cx_asset_package_record** pp_out) {

	const uint8_t asset_type = CX_ASSET_GET_TYPE_ID(id);

	struct hashtable_itr itr;
	
	if (hashtable_i_find(&p_package->asset_type_record_tables_[asset_type], id, &itr)) {
		*pp_out = itr.p_value;
		return CX_TRUE;
	}

	return CX_FALSE;
}

int cx_asset_package_get_asset_name(const struct cx_asset_package* p_package, cx_asset_id id, const char** pp_out) {
	const struct cx_asset_package_record* p_record;
	if (!cx_asset_package_find_record(p_package, id, &p_record)) {
		return CX_FALSE;
	}

	*pp_out = p_record->name;
	return CX_TRUE;
}

int cx_asset_package_find_asset_by_name(
	const struct cx_asset_package* p_package, cx_asset_type type, const char* s_name, struct cx_asset_ref* p_out_ref) {

	struct hashtable_itr itr;
	hashtable_itr(&p_package->asset_type_record_tables_[type], &itr);

	while (hashtable_itr_is_valid(&itr)) {
		const struct cx_asset_package_record* p_record = itr.p_value;

		if (strcmp(s_name, p_record->name) == 0) {
			*p_out_ref = (struct cx_asset_ref) { .asset_id = *(const cx_asset_id*)itr.p_key };
			return CX_TRUE;
		}

		hashtable_itr_next(&itr);
	}

	return CX_FALSE;
}

int cx_asset_package_deserialize_asset(const struct cx_asset_package* p_package, cx_asset_id id, void* p_out) {
	const struct cx_asset_package_record* p_record;
	if (!cx_asset_package_find_record(p_package, id, &p_record)) {
		return CX_FALSE;
	}

	struct cx_stream_file stream;

	if (!cx_stream_file_open(p_package->s_filename_, "rb", &stream)) {
		return CX_FALSE;
	}

	cx_stream_seek(&stream.base, p_record->file_location_, CX_STREAM_SEEK_ORIGIN_begin);

	const int b_result = cx_asset_type_deserialize_asset(CX_ASSET_GET_TYPE_ID(id), &stream.base, p_out);

	cx_stream_close(&stream.base);

	return b_result;
}
