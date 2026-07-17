#ifndef CX_ASSET_PACKAGE_H
#define CX_ASSET_PACKAGE_H

#define CX_ASSET_PACKAGE_FILENAME_MAX_LEN 260

struct cx_asset_package {
	char s_filename_[CX_ASSET_PACKAGE_FILENAME_MAX_LEN];
	struct hashtable asset_type_record_tables_[CX_ASSET_TYPE_ID_MAX];
};

struct cx_asset_package_record {
	struct cx_asset asset_;
	struct cx_asset_package* p_package_;
	uint32_t file_location_;
};

void cx_asset_package_free(struct cx_asset_package* p_package);

int cx_asset_package_deserialize_records(struct cx_asset_package* p_package, struct cx_stream* p_stream);

void cx_asset_package_load_records_from_file(struct cx_asset_package* p_package, const char* s_filename);

int cx_asset_package_find_record(
	const struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out);

#endif
