#ifndef CX_ASSET_PACKAGE_H
#define CX_ASSET_PACKAGE_H

#include <stddef.h>
#include <stdint.h>

#include "cx_asset_defs.h"
#include "hashtable.h"

#define CX_LOG_CAT_ASSET_PACKAGE "asset:pkg"

#define CX_ASSET_PACKAGE_FILENAME_MAX_LEN 260

struct cx_asset_package {
	char s_filename_[CX_ASSET_PACKAGE_FILENAME_MAX_LEN];
	struct hashtable asset_type_record_tables_[CX_ASSET_TYPE_ID_MAX];
};

struct cx_asset_package_record {
	struct cx_asset_package* p_package_;
	char name[CX_ASSET_NAME_MAX_LEN];
	uint32_t file_location_;
};

struct cx_stream;

int cx_asset_package_import(const char* s_filename, struct cx_asset_package* p_out);

void cx_asset_package_free(struct cx_asset_package* p_package);

int cx_asset_package_deserialize_records(struct cx_asset_package* p_package, struct cx_stream* p_stream);

int cx_asset_package_find_record(
	const struct cx_asset_package* p_package,
	cx_asset_id id,
	const struct cx_asset_package_record** pp_out);

int cx_asset_package_get_asset_name(const struct cx_asset_package* p_package, cx_asset_id id, const char** pp_out);

int cx_asset_package_find_asset_by_name(
	const struct cx_asset_package* p_package, cx_asset_type type, const char* s_name, struct cx_asset_ref* p_out_ref);

int cx_asset_package_deserialize_asset(const struct cx_asset_package* p_package, cx_asset_id id, void* p_out);

#endif
