#include <string.h>

#include "cx_asset.h"
#include "cx_asset_cache.h"
#include "cx_ed_asset.h"
#include "cx_ed_asset_library.h"
#include "cx_stream_file.h"
#include "cx_stream_serialization.h"
#include "hashtable.h"

struct cx_ed_asset_library_entry {
	struct cx_asset_ref asset_ref;
	char name[CX_ASSET_NAME_MAX_LEN + 1];
	char filepath[250];
	size_t asset_file_offset;
	int b_is_dirty;
};

static struct hashtable library_asset_tables[CX_ASSET_TYPE_ID_MAX];

void cx_ed_asset_library_add_file(const char* s_filepath) {
	struct cx_stream_file stream;

	cx_stream_file_open(s_filepath, "rb", &stream);

	cx_asset_id asset_id;
	cx_stream_deserialize_uint32(&stream.base, &asset_id);

	const cx_asset_type type = CX_ASSET_GET_TYPE_ID(asset_id);

	if (library_asset_tables[type].element_size_ == 0) {
		hashtable_init(&library_asset_tables[type], sizeof(struct cx_ed_asset_library_entry));
	}

	struct cx_ed_asset_library_entry* p_new_entry = hashtable_i_add(&library_asset_tables[type], asset_id);
	*p_new_entry = (struct cx_ed_asset_library_entry){
		.asset_file_offset = cx_stream_tell(&stream.base)
	};
	
	size_t name_len;
	cx_stream_deserialize_cstring(&stream.base, p_new_entry->name, &name_len);

	strcpy(p_new_entry->filepath, s_filepath);

	cx_stream_close(&stream.base);
}

void cx_ed_asset_library_new(cx_asset_type type, const char* s_name, void* p_asset, struct cx_asset_ref* p_out) {
	cx_asset_id new_asset_id = cx_ed_asset_generate_id(type);

	if (library_asset_tables[type].element_size_ == 0) {
		hashtable_init(&library_asset_tables[type], sizeof(struct cx_ed_asset_library_entry));
	}

	struct cx_ed_asset_library_entry* p_new_entry = hashtable_i_add(&library_asset_tables[type], new_asset_id);
	*p_new_entry = (struct cx_ed_asset_library_entry) {
		.b_is_dirty = CX_TRUE
	};

	strcpy(p_new_entry->name, s_name);

	CX_LOG_FMT(INFO, ASSET, "New asset: %s\n", p_new_entry->name);

	cx_asset_cache_adopt(new_asset_id, p_asset, &p_new_entry->asset_ref);

	*p_out = p_new_entry->asset_ref;
}

void cx_ed_asset_library_delete(cx_asset_id id) {
	
}

void cx_ed_asset_library_make_dirty(cx_asset_id id) {
}

void cx_ed_asset_library_save(cx_asset_id id) {
}

int cx_ed_asset_library_get_asset_name(cx_asset_id id, const char** pp_out) {
	const cx_asset_type type = CX_ASSET_GET_TYPE_ID(id);

	if (library_asset_tables[type].element_size_ == 0) {
		return CX_FALSE;
	}

	struct hashtable_itr itr;
	if (!hashtable_i_find(&library_asset_tables[type], id, &itr)) {
		return CX_FALSE;
	}

	const struct cx_ed_asset_library_entry* p_entry = itr.p_value;

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
	const cx_asset_type type = CX_ASSET_GET_TYPE_ID(id);

	if (library_asset_tables[type].element_size_ == 0) {
		return CX_FALSE;
	}

	struct hashtable_itr itr;
	if (!hashtable_i_find(&library_asset_tables[type], id, &itr)) {
		return CX_FALSE;
	}

	const struct cx_ed_asset_library_entry* p_entry = itr.p_value;

	struct cx_stream_file stream;
	
	if (!cx_stream_file_open(p_entry->filepath, "rb", &stream)) {
		return CX_FALSE;
	}
	
	cx_stream_seek(&stream.base, (ptrdiff_t)p_entry->asset_file_offset, CX_STREAM_SEEK_ORIGIN_begin);
	
	const int b_result = cx_asset_type_deserialize_asset(type, &stream.base, p_out);
	
	cx_stream_close(&stream.base);

	return b_result;
}

void cx_ed_asset_library_free(void) {
	for (cx_asset_type i = 0; i < CX_ASSET_TYPE_ID_MAX; ++i) {
		hashtable_free(&library_asset_tables[i]);
	}
}
