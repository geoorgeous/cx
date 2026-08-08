#include <string.h>

#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_ed_asset.h"
#include "cx_ed_asset_library.h"
#include "cx_io.h"
#include "cx_str.h"
#include "cx_stream_file.h"
#include "cx_stream_serialization.h"
#include "hashtable.h"

static struct hashtable library_asset_tables[CX_ASSET_TYPE_ID_MAX];

static int cx_ed_asset_library_find_entry(cx_asset_id id, struct cx_ed_asset_library_entry** pp_out_entry);

static cx_result cx_ed_asset_library_save_asset_internal(struct cx_ed_asset_library_entry* p_entry);

int cx_ed_asset_library_add_file(const char* s_filepath, struct cx_asset_ref* p_out) {
	struct cx_stream_file stream;

	if (!cx_stream_file_open(s_filepath, "rb", &stream)) {
		return CX_FALSE;
	}

	uint8_t sig[sizeof(CX_ED_IMPORT_FILE_SIG_ASSET)];
	cx_stream_deserialize_bytes(&stream.base, sizeof(sig), sig);

	if (memcmp(CX_ED_IMPORT_FILE_SIG_ASSET, sig, sizeof(sig)) != 0) {
		// not an asset file/file malformed
		return CX_FALSE;
	}

	cx_asset_id asset_id;
	cx_stream_deserialize_uint32(&stream.base, &asset_id);

	const cx_asset_type type = CX_ASSET_GET_TYPE_ID(asset_id);

	if (library_asset_tables[type].element_size_ == 0) {
		hashtable_init(&library_asset_tables[type], sizeof(struct cx_ed_asset_library_entry));
	}

	struct cx_ed_asset_library_entry* p_new_entry = hashtable_i_add(&library_asset_tables[type], asset_id);

	if (p_new_entry == CX_NULL) {
		return CX_FALSE;
	}

	*p_new_entry = (struct cx_ed_asset_library_entry){
		.asset_ref = { .asset_id = asset_id },
	};
	
	size_t name_len;
	cx_stream_deserialize_cstring(&stream.base, p_new_entry->name, &name_len);

	p_new_entry->asset_file_offset = cx_stream_tell(&stream.base);

	strcpy(p_new_entry->filepath, s_filepath);

	cx_stream_close(&stream.base);

	*p_out = (struct cx_asset_ref) { .asset_id = p_new_entry->asset_ref.asset_id };

	return CX_TRUE;
}

static int cx_ed_asset_library_scan_dir_enumerate_cb(struct cx_io_dir_entry* p_entry, void* p_user_ptr) {
	(void)p_user_ptr;

	if (p_entry->type == CX_IO_DIR_ENTRY_TYPE_DIR) {
		return CX_TRUE;
	}

	char filepath[256];
	cx_io_filepath_join(p_entry->p_dir->s_dir, p_entry->s_name, filepath);

	struct cx_asset_ref ref;
	cx_ed_asset_library_add_file(filepath, &ref);

	return CX_TRUE;
}

void cx_ed_asset_library_scan_dir(const char* s_dir) {
	cx_io_dir_enumerate(s_dir, cx_ed_asset_library_scan_dir_enumerate_cb, CX_NULL);
}

int cx_ed_asset_library_import_asset_package(const char* s_filepath) {
	return CX_FALSE;
}

void cx_ed_asset_library_new(cx_asset_type type, const char* s_name, void* p_asset, struct cx_asset_ref* p_out) {
	cx_asset_id new_asset_id = cx_ed_asset_generate_id(type);

	if (library_asset_tables[type].element_size_ == 0) {
		hashtable_init(&library_asset_tables[type], sizeof(struct cx_ed_asset_library_entry));
	}

	struct cx_ed_asset_library_entry* p_new_entry = hashtable_i_add(&library_asset_tables[type], new_asset_id);
	*p_new_entry = (struct cx_ed_asset_library_entry) {
		.b_unsaved = CX_TRUE
	};

	strcpy(p_new_entry->name, s_name);

	CX_LOG_FMT(INFO, ASSET, "New asset: %s\n", p_new_entry->name);

	cx_asset_cache_adopt(new_asset_id, p_asset, &p_new_entry->asset_ref);

	*p_out = (struct cx_asset_ref) { .asset_id = new_asset_id };
}

void cx_ed_asset_library_delete(cx_asset_id id) {
	
}

void cx_ed_asset_library_set_filepath(cx_asset_id id, const char* s_filepath) {
	struct cx_ed_asset_library_entry* p_entry;
	if (!cx_ed_asset_library_find_entry(id, &p_entry)) {
		return;
	}
	strcpy(p_entry->filepath, s_filepath);
	CX_LOG_FMT(INFO, ASSET, "Set asset %X filepath to %s\n", id, p_entry->filepath);
}

cx_result cx_ed_asset_library_save_asset_internal(struct cx_ed_asset_library_entry* p_entry) {
	if (cx_str_is_empty(p_entry->filepath)) {
		return CX_ERROR_ASSET_NO_FILEPATH;
	}

	struct cx_stream_file stream;
	if (!cx_stream_file_open(p_entry->filepath, "wb", &stream)) {
		return CX_ERROR_IO;
	}

	cx_stream_serialize_bytes(&stream.base, sizeof(CX_ED_IMPORT_FILE_SIG_ASSET), CX_ED_IMPORT_FILE_SIG_ASSET);

	cx_stream_serialize_uint32(&stream.base, p_entry->asset_ref.asset_id);

	cx_stream_serialize_string(&stream.base, p_entry->name, 0);

	cx_asset_cache_acquire(&p_entry->asset_ref);

	cx_asset_type_serialize_asset(
		CX_ASSET_GET_TYPE_ID(p_entry->asset_ref.asset_id), *p_entry->asset_ref.pp_asset, &stream.base);

	cx_asset_cache_release(&p_entry->asset_ref);

	cx_stream_close(&stream.base);

	p_entry->b_unsaved = CX_FALSE;

	return CX_SUCCESS;
}

cx_result cx_ed_asset_library_save(cx_asset_id id) {
	struct cx_ed_asset_library_entry* p_entry;
	if (!cx_ed_asset_library_find_entry(id, &p_entry)) {
		return CX_ERROR_NOT_FOUND;
	}
	return cx_ed_asset_library_save_asset_internal(p_entry);
}

cx_result cx_ed_asset_library_save_as(cx_asset_id id, const char* s_filepath) {
	struct cx_ed_asset_library_entry* p_entry;
	if (!cx_ed_asset_library_find_entry(id, &p_entry)) {
		return CX_ERROR_NOT_FOUND;
	}

	CX_ASSERT(s_filepath != CX_NULL, ASSET);
	CX_ASSERT(!cx_str_is_empty(s_filepath), ASSET);

	if (strcmp(p_entry->filepath, s_filepath) != 0 && cx_io_filepath_exists(s_filepath)) {
		return CX_ERROR_ALREADY_EXISTS;
	}

	strcpy(p_entry->filepath, s_filepath);

	return cx_ed_asset_library_save_asset_internal(p_entry);
}

void cx_ed_asset_library_save_all_unsaved(void) {
	for (size_t t = 0; t < CX_ASSET_TYPE_ID_MAX; ++t) {
		struct hashtable_itr itr;
		hashtable_itr(&library_asset_tables[t], &itr);

		while (hashtable_itr_is_valid(&itr)) {
			struct cx_ed_asset_library_entry* p_entry = itr.p_value;
			if (p_entry->b_unsaved) {
				cx_ed_asset_library_save_asset_internal(p_entry);
			}
			hashtable_itr_next(&itr);
		}
	}
}

int cx_ed_asset_library_get_asset_name(cx_asset_id id, const char** pp_out) {
	struct cx_ed_asset_library_entry* p_entry;
	if (!cx_ed_asset_library_find_entry(id, &p_entry)) {
		return CX_FALSE;
	}

	*pp_out = p_entry->name;
	return CX_TRUE;
}

int cx_ed_asset_library_find_asset_by_name(cx_asset_type type, const char* s_name, struct cx_asset_ref* p_out) {
	struct hashtable_itr itr;
	hashtable_itr(&library_asset_tables[type], &itr);

	while (hashtable_itr_is_valid(&itr)) {
		const struct cx_ed_asset_library_entry* p_entry = itr.p_value;

		CX_LOG_FMT(INFO, ASSET, "Comparing asset name: %s == %s\n", s_name, p_entry->name);

		if (strcmp(s_name, p_entry->name) == 0) {
			*p_out = (struct cx_asset_ref) { .asset_id = p_entry->asset_ref.asset_id };
			return CX_TRUE;
		}

		hashtable_itr_next(&itr);
	}

	return CX_FALSE;
}

int cx_ed_asset_library_deserialize_asset(cx_asset_id id, void* p_out) {
	struct cx_ed_asset_library_entry* p_entry;
	if (!cx_ed_asset_library_find_entry(id, &p_entry)) {
		return CX_FALSE;
	}

	struct cx_stream_file stream;
	
	if (!cx_stream_file_open(p_entry->filepath, "rb", &stream)) {
		return CX_FALSE;
	}
	
	cx_stream_seek(&stream.base, (ptrdiff_t)p_entry->asset_file_offset, CX_STREAM_SEEK_ORIGIN_begin);
	
	const int b_result = cx_asset_type_deserialize_asset(CX_ASSET_GET_TYPE_ID(id), &stream.base, p_out);
	
	cx_stream_close(&stream.base);

	return b_result;
}

void cx_ed_asset_library_enumerate_assets(cx_ed_asset_library_enumerate_assets_cb_fn f_cb, void* p_user_ptr) {
	for (cx_asset_type t = 0; t < CX_ASSET_TYPE_ID_MAX; ++t) {
		if (library_asset_tables[t].element_size_ != 0) {
			cx_ed_asset_library_enumerate_assets_of_type(t, f_cb, p_user_ptr);
		}
	}
}

void cx_ed_asset_library_enumerate_assets_of_type(
	cx_asset_type type, cx_ed_asset_library_enumerate_assets_cb_fn f_cb, void* p_user_ptr) {

	struct hashtable_itr itr;
	hashtable_itr(&library_asset_tables[type], &itr);

	while (hashtable_itr_is_valid(&itr)) {
		const struct cx_ed_asset_library_entry* p_entry = itr.p_value;
		f_cb(p_entry, p_user_ptr);
		hashtable_itr_next(&itr);
	}
}

void cx_ed_asset_library_free(void) {
	for (cx_asset_type i = 0; i < CX_ASSET_TYPE_ID_MAX; ++i) {
		hashtable_free(&library_asset_tables[i]);
	}
}

int cx_ed_asset_library_find_entry(cx_asset_id id, struct cx_ed_asset_library_entry** pp_out_entry) {
	const cx_asset_type type = CX_ASSET_GET_TYPE_ID(id);

	if (library_asset_tables[type].element_size_ == 0) {
		return CX_FALSE;
	}

	struct hashtable_itr itr;
	if (!hashtable_i_find(&library_asset_tables[type], id, &itr)) {
		return CX_FALSE;
	}

	*pp_out_entry = itr.p_value;
	return CX_TRUE;
}
