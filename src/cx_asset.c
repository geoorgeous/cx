#include <string.h>

#include "cx_asset.h"
#include "cx_logging.h"
#include "cx_stream_serialization.h"

#define CX_ASSET_TYPE_NAME_MAX_LEN 63

static struct asset_type_table {
	size_t asset_size;
	cx_asset_serialize_fn f_serialize;
	cx_asset_deserialize_fn f_deserialize;
	cx_asset_enumerate_dependencies_fn f_enumerate_dependencies;
	cx_asset_free_fn f_free;
	char s_display_name[CX_ASSET_TYPE_NAME_MAX_LEN + 1];
} asset_type_tables[CX_ASSET_TYPE_ID_MAX];

void cx_asset_register_type(
	cx_asset_type type,
	const char* s_display_name,
	size_t size,
	cx_asset_serialize_fn f_serialize,
	cx_asset_deserialize_fn f_deserialize,
	cx_asset_enumerate_dependencies_fn f_enumerte_dependencies,
	cx_asset_free_fn f_free) {

	if (asset_type_tables[type].f_serialize) {
		CX_LOG_FMT(ERROR, ASSET, "Asset type id %d already registered by type '%s'.\n",
			type, asset_type_tables[type].s_display_name);
		return;
	}

	CX_LOG_FMT(INFO, ASSET, "Asset type registered: { id=%d, name='%s' }\n", type, s_display_name);

	strcpy(asset_type_tables[type].s_display_name, s_display_name);
	asset_type_tables[type].asset_size = size;
	asset_type_tables[type].f_serialize = f_serialize;
	asset_type_tables[type].f_deserialize = f_deserialize;
	asset_type_tables[type].f_enumerate_dependencies = f_enumerte_dependencies;
	asset_type_tables[type].f_free = f_free;
}

const char* cx_asset_type_display_name_str(cx_asset_type type) {
	return asset_type_tables[type].s_display_name;
}

size_t cx_asset_type_size(cx_asset_type type) {
	return asset_type_tables[type].asset_size;
}

int cx_asset_type_serialize_asset(cx_asset_type type, const void* p_asset, struct cx_stream* p_stream) {
	return asset_type_tables[type].f_serialize(p_asset, p_stream);
}

int cx_asset_type_deserialize_asset(cx_asset_type type, struct cx_stream* p_stream, void* p_asset) {
	return asset_type_tables[type].f_deserialize(p_stream, p_asset);
}

void cx_asset_type_enumerate_dependencies(
	cx_asset_type type, const void* p_asset, cx_asset_enumerate_dependencies_cb_fn f_cb, void* p_user_ptr) {
	if (asset_type_tables[type].f_enumerate_dependencies) {
		asset_type_tables[type].f_enumerate_dependencies(p_asset, f_cb, p_user_ptr);
	}
}

void cx_asset_type_free_asset(cx_asset_type type, void* p_asset) {
	if (asset_type_tables[type].f_free) {
		asset_type_tables[type].f_free(p_asset);
	}
}

void* cx_asset_ref_get(const struct cx_asset_ref* p_ref) {
	if (cx_asset_ref_is_valid(p_ref)) {
		return *p_ref->pp_asset;
	}
	return CX_NULL;
}

int cx_asset_ref_is_valid(const struct cx_asset_ref* p_ref) {
	return p_ref->pp_asset != CX_NULL;
}

int cx_asset_ref_is_set(const struct cx_asset_ref* p_ref) {
	return p_ref->asset_id != 0;
}

int cx_asset_ref_serialize(const struct cx_asset_ref* p_ref, struct cx_stream* p_stream) {
	return cx_stream_serialize_uint32(p_stream, p_ref->asset_id);
}

int cx_asset_ref_deserialize(struct cx_stream* p_stream, struct cx_asset_ref* p_out) {
	*p_out = (struct cx_asset_ref){0};
	return cx_stream_deserialize_uint32(p_stream, &p_out->asset_id);
}
