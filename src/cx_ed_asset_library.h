#ifndef CX_ED_ASSET_LIBRARY_H
#define CX_ED_ASSET_LIBRARY_H

#include "cx_asset_types.h"

void cx_ed_asset_library_add_file(const char* s_filepath);
void cx_ed_asset_library_new(cx_asset_type type, void* p_asset, struct cx_asset_ref* p_out);
void cx_ed_asset_library_delete(cx_asset_id id);
void cx_ed_asset_library_make_dirty(cx_asset_id id);
void cx_ed_asset_library_save(cx_asset_id id);
int cx_ed_asset_library_deserialize_asset(cx_asset_id id, void* p_out);
void cx_ed_asset_library_free(void);

#endif
