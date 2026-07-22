#include "cx_asset_store.h"
#include "cx_ed_asset_database.h"
#include "cx_macro.h"
#include "hashtable.h"

struct cx_ed_asset_database_record {
	struct cx_asset_store_record base;
	const char* s_filepath;
};

static struct hashtable asset_type_record_tables[CX_ASSET_TYPE_ID_MAX];

void cx_ed_asset_database_new(cx_asset_type type, void* p_asset, cx_asset_handle* p_out_new_handle) {
	if (asset_type_record_tables[type].element_size_ == 0) {
		hashtable_init(&asset_type_record_tables[type], sizeof(struct cx_ed_asset_database_record));
	}

	cx_asset_id new_asset_id; // todo
	
	struct cx_ed_asset_database_record* p_new_asset_record = hashtable_i_add(&asset_type_record_tables[type], new_asset_id);
	*p_new_asset_record = (struct cx_ed_asset_database_record) {
		.base = {
			.p_asset = p_asset,
			.id = new_asset_id,
			.p_source = 0 // todo
		}
	};

	*p_out_new_handle = &p_new_asset_record->base;
}

void cx_ed_asset_database_delete(cx_asset_handle* p_handle) {
}

int cx_ed_asset_database_find_asset(cx_asset_id asset_id, cx_asset_handle* p_out_handle) {
	const struct hashtable* p_records = &asset_type_record_tables[CX_ASSET_GET_TYPE_ID(asset_id)];

	if (p_records->element_size_ == 0) {
		return CX_FALSE;
	}

	struct hashtable_itr itr;
	if (!hashtable_i_find(p_records, asset_id, &itr)) {
		return CX_FALSE;
	}

	*p_out_handle = itr.p_value;
	return CX_TRUE;
}

int cx_ed_asset_database_open_asset_read_stream(uint32_t asset_id, struct cx_stream* p_out) {
	cx_asset_handle handle;
	if (!cx_ed_asset_database_find_asset(asset_id, &handle)) {
		return CX_FALSE;
	}

	const struct cx_ed_asset_database_record* p_record = (void*)handle;

}

void cx_ed_asset_database_save_asset(cx_asset_id asset_id) {
}

void cx_ed_asset_database_save_asset_as(cx_asset_id asset_id, const char* s_filepath) {
}

void cx_ed_asset_database_unload_all(void) {
}
