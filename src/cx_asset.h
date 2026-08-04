#ifndef ASSET_H
#define ASSET_H

#include <stddef.h>
#include <stdint.h>

#include "cx_asset_types.h"

struct cx_stream;

void cx_asset_register_type(
	cx_asset_type type,
	const char* s_display_name,
	size_t size,
	cx_asset_serialize_fn f_serialize,
	cx_asset_deserialize_fn f_deserialize,
	cx_asset_enumerate_dependencies_fn f_enumerate_dependencies,
	cx_asset_free_fn f_free);

const char* cx_asset_type_display_name_str(cx_asset_type type);
size_t cx_asset_type_size(cx_asset_type type);
int cx_asset_type_serialize_asset(cx_asset_type type, const void* p_asset, struct cx_stream* p_stream);
int cx_asset_type_deserialize_asset(cx_asset_type type, struct cx_stream* p_stream, void* p_asset);
void cx_asset_type_enumerate_dependencies(
	cx_asset_type type, const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr);
void cx_asset_type_free_asset(cx_asset_type type, void* p_asset);

void* cx_asset_ref_get(const struct cx_asset_ref* p_ref);
int cx_asset_ref_is_valid(const struct cx_asset_ref* p_ref);
int cx_asset_ref_is_set(const struct cx_asset_ref* p_ref);
int cx_asset_ref_serialize(const struct cx_asset_ref* p_ref, struct cx_stream* p_stream);
int cx_asset_ref_deserialize(struct cx_stream* p_stream, struct cx_asset_ref* p_out);

#endif
