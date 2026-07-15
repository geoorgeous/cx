#ifndef ASSET_H
#define ASSET_H

#include <stddef.h>
#include <stdint.h>

#include "hashtable.h"

#define CX_LOG_CAT_ASSET "asset"

#define CX_ASSET_PACKAGE_FILENAME_MAX_LEN 260
#define CX_ASSET_TYPE_ID_MAX 0xFF
#define CX_ASSET_IDN_MAX 0x00FFFFFF
#define CX_ASSET_IDN_MASK CX_ASSET_IDN_MAX
#define CX_ASSET_MAKE_ID(TYPE, IDN) ((uint32_t)(((uint8_t)(TYPE)) << 24) | (((int)(IDN)) & CX_ASSET_IDN_MASK))
#define CX_ASSET_GET_TYPE_ID(ID) ((uint8_t)((ID) >> 24))
#define CX_ASSET_GET_IDN(ID) (((uint32_t)(ID)) & CX_ASSET_IDN_MASK)

struct cx_stream;

typedef uint32_t cx_asset_id;

struct cx_asset {
	void* p_data_;
	cx_asset_id id_;
};

typedef int(*cx_asset_serialize_fn)(const void*, struct cx_stream*);
typedef int(*cx_asset_deserialize_fn)(struct cx_stream*, void*);
typedef void(*cx_asset_free_fn)(void*);

void cx_asset_register_type(
	uint8_t asset_type_id,
	const char* s_display_name,
	size_t size,
	cx_asset_serialize_fn f_serialize,
	cx_asset_deserialize_fn f_deserialize,
	cx_asset_free_fn f_free);

struct cx_asset_package_record {
	struct cx_asset asset_;
	struct cx_asset_package* p_package_;
	uint32_t file_location_;
};

typedef struct cx_asset_package_record* cx_asset_handle;

int  cx_asset_load(cx_asset_handle p_record);
void cx_asset_free(cx_asset_handle p_record);

struct cx_asset_package {
	char s_filename_[CX_ASSET_PACKAGE_FILENAME_MAX_LEN];
	struct hashtable asset_type_record_tables_[CX_ASSET_TYPE_ID_MAX];
};

void cx_asset_package_free(struct cx_asset_package* p_package);

int cx_asset_package_serialize(const struct cx_asset_package* p_package, struct cx_stream* p_stream);

int cx_asset_package_deserialize_records(struct cx_asset_package* p_package, struct cx_stream* p_stream);

void cx_asset_package_load_records_from_file(struct cx_asset_package* p_package, const char* s_filename);

void cx_asset_package_save_to_file(struct cx_asset_package* p_package);

void cx_asset_package_save_to_file_as(struct cx_asset_package* p_package, const char* s_filename);

int cx_asset_package_find_record(
	const struct cx_asset_package* p_package,
	cx_asset_id id,
	struct cx_asset_package_record** pp_out);

void cx_asset_package_new_record(
	struct cx_asset_package* p_package,
	uint8_t type,
	struct cx_asset_package_record** pp_out);

void cx_asset_package_delete_record(struct cx_asset_package* p_package, cx_asset_id id);

void cx_asset_directory_register_package(const struct cx_asset_package* p_package);

int cx_asset_directory_find(cx_asset_id id, struct cx_asset_package_record** pp_out);

const struct cx_asset_package** cx_asset_directory_get_packages(size_t* p_num_packages);

int cx_asset_handle_serialize(const cx_asset_handle p_asset_handle, struct cx_stream* p_stream);

int cx_asset_handle_deserialize(struct cx_stream* p_stream, cx_asset_handle* p_out_asset_handle);

#endif
