#ifndef CX_ASSET_STORE_H
#define CX_ASSET_STORE_H

#include "cx_asset_types.h"

struct cx_asset_store_record {
	cx_asset_id id;
	struct cx_asset_store* p_source;
	void* p_asset;
};

struct cx_stream;

typedef int(*cx_asset_store_find_asset_fn)(cx_asset_id, cx_asset_handle*);
typedef int(*cx_asset_store_open_asset_read_stream_fn)(cx_asset_id, struct cx_stream*);

struct cx_asset_store {
	cx_asset_store_find_asset_fn f_find_asset;
	cx_asset_store_open_asset_read_stream_fn f_open_asset_read_stream;
};

static inline int cx_asset_store_find_asset(
	const struct cx_asset_store* p_store,
	cx_asset_id asset_id,
	cx_asset_handle* p_out_handle) {

	return p_store->f_find_asset(asset_id, p_out_handle);
}

static inline int cx_asset_store_open_asset_read_stream(
	const struct cx_asset_store* p_store,
	cx_asset_id asset_id,
	struct cx_stream* p_out) {

	return p_store->f_open_asset_read_stream(asset_id, p_out);
}

#endif
