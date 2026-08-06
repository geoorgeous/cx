#ifndef CX_ED_ASSET_LIBRARY_H
#define CX_ED_ASSET_LIBRARY_H

#include <stddef.h>

#include "cx_asset_types.h"

struct cx_ed_asset_library_entry {
	struct cx_asset_ref asset_ref;
	char name[CX_ASSET_NAME_MAX_LEN + 1];
	char filepath[250];
	size_t asset_file_offset;
	int b_unsaved;
};

typedef void(*cx_ed_asset_library_enumerate_assets_cb_fn)(const struct cx_ed_asset_library_entry*, void*);

int cx_ed_asset_library_add_file(const char* s_filepath, struct cx_asset_ref* p_out);
int cx_ed_asset_library_import_asset_package(const char* s_filepath);
void cx_ed_asset_library_new(cx_asset_type type, const char* s_name, void* p_asset, struct cx_asset_ref* p_out);
void cx_ed_asset_library_delete(cx_asset_id id);
void cx_ed_asset_library_make_dirty(cx_asset_id id);
void cx_ed_asset_library_save(cx_asset_id id);
int cx_ed_asset_library_get_asset_name(cx_asset_id id, const char** pp_out);
int cx_ed_asset_library_find_asset_by_name(cx_asset_type type, const char* s_name, struct cx_asset_ref* p_out);
int cx_ed_asset_library_deserialize_asset(cx_asset_id id, void* p_out);
void cx_ed_asset_library_enumerate_assets(cx_ed_asset_library_enumerate_assets_cb_fn f_cb, void* p_user_ptr);
void cx_ed_asset_library_enumerate_assets_of_type(
	cx_asset_type type, cx_ed_asset_library_enumerate_assets_cb_fn f_cb, void* p_user_ptr);
void cx_ed_asset_library_free(void);

#endif
