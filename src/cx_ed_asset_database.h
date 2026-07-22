#ifndef CX_ED_ASSET_DATABASE_H
#define CX_ED_ASSET_DATABASE_H

#include "cx_asset_types.h"

struct cx_stream;

void cx_ed_asset_database_new(cx_asset_type type, void* p_asset, cx_asset_handle* p_out_new_handle);
void cx_ed_asset_database_delete(cx_asset_handle* p_handle);
int cx_ed_asset_database_find_asset(cx_asset_id asset_id, cx_asset_handle* p_out_handle);
int cx_ed_asset_database_open_asset_read_stream(cx_asset_id asset_id, struct cx_stream* p_out);
void cx_ed_asset_database_save_asset(cx_asset_id asset_id);
void cx_ed_asset_database_save_asset_as(cx_asset_id asset_id, const char* s_filepath);
void cx_ed_asset_database_unload_all(void);

#endif
