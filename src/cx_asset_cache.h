#ifndef CX_ASSET_CACHE_H
#define CX_ASSET_CACHE_H

#include "cx_asset_types.h"

struct cx_asset_source {
	void* p_context;
	int (*f_try_deserialize_asset)(cx_asset_id, void*, void*);
};

void cx_asset_cache_push_source(const struct cx_asset_source* p_source);
void cx_asset_cache_remove_source(const struct cx_asset_source* p_source);
void cx_asset_cache_adopt(cx_asset_id asset_id, void* p_asset, struct cx_asset_ref* p_out);
void* cx_asset_cache_acquire(struct cx_asset_ref* p_ref);
void cx_asset_cache_release(struct cx_asset_ref* p_ref);
void cx_asset_cache_free(void);

#endif
