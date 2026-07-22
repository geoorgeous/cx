#ifndef ASSET_H
#define ASSET_H

#include <stddef.h>
#include <stdint.h>

#include "cx_asset_types.h"

struct cx_asset_store;

struct cx_stream;

typedef int(*cx_asset_serialize_fn)(const void*, struct cx_stream*);
typedef int(*cx_asset_deserialize_fn)(struct cx_stream*, void*);
typedef void(*cx_asset_free_fn)(void*);

void cx_asset_register_type(
	cx_asset_type type,
	const char* s_display_name,
	size_t size,
	cx_asset_serialize_fn f_serialize,
	cx_asset_deserialize_fn f_deserialize,
	cx_asset_free_fn f_free);

int cx_asset_load(cx_asset_handle handle);
void cx_asset_free(cx_asset_handle handle);
void* cx_asset_get(cx_asset_handle handle);
cx_asset_id cx_asset_generate_id(cx_asset_type type);

#endif
