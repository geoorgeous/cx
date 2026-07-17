#ifndef CX_ED_ASSET_DATABASE_H
#define CX_ED_ASSET_DATABASE_H

#include <stdint.h>

struct cx_asset_ref;
struct cx_stream;

void cx_ed_asset_database_new(void);
void cx_ed_asset_database_delete(void);
void cx_ed_asset_database_find_asset(uint32_t asset_id, struct cx_asset_ref* p_out);
void cx_ed_asset_database_open_asset_read_stream(uint32_t, struct cx_stream* p_out);

#endif
