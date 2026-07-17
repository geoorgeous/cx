#ifndef CX_ASSET_STORE_H
#define CX_ASSET_STORE_H

#include <stdint.h>

struct cx_asset_store;

struct cx_asset_store_record {
	
};

struct cx_asset_ref;

void cx_asset_store_find_asset(const struct cx_asset_store* p_store, uint32_t asset_id, struct cx_asset_ref* p_out);
void cx_asset_store_open_asset_read_stream(const struct cx_asset_store*, uint32_t asset_id, struct cx_stream* p_out);

#endif
