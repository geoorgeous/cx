#include <string.h>

#include "cx_asset.h"
#include "cx_asset_store.h"
#include "cx_logging.h"
#include "cx_stream.h"
#include "cx_stream_serialization.h"

#define CX_ASSET_TYPE_NAME_MAX_LEN 63

static struct asset_type_table {
	size_t asset_size;
	cx_asset_serialize_fn f_serialize;
	cx_asset_deserialize_fn f_deserialize;
	cx_asset_free_fn f_free;
	char s_display_name[CX_ASSET_TYPE_NAME_MAX_LEN + 1];
} asset_type_tables[CX_ASSET_TYPE_ID_MAX];

void cx_asset_register_type(
	cx_asset_type type,
	const char* s_display_name,
	size_t size,
	cx_asset_serialize_fn f_serialize,
	cx_asset_deserialize_fn f_deserialize,
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
	asset_type_tables[type].f_free = f_free;
}

int cx_asset_load(cx_asset_handle handle) {
	if (!handle->p_source) {
		CX_LOG_FMT(ERROR, ASSET, "Failed to load asset %x: asset has no source\n",
			handle->id);
		return CX_FALSE;
	}

	struct cx_stream stream;
	if (!cx_asset_store_open_asset_read_stream(handle->p_source, handle->id, &stream)) {
		return CX_FALSE;
	}

	const struct asset_type_table* p_type_table = &asset_type_tables[CX_ASSET_GET_TYPE_ID(handle->id)];

	handle->p_asset = calloc(1, p_type_table->asset_size);

	const int b_result = p_type_table->f_deserialize(&stream, handle->p_asset);

	cx_stream_close(&stream);

	return b_result;
}

void cx_asset_free(cx_asset_handle handle) {
	if (!handle->p_asset) {
		return;
	}

	CX_LOG_FMT(INFO, ASSET, "Unloading asset (%s) %x\n", 
		asset_type_tables[CX_ASSET_GET_TYPE_ID(handle->id)].s_display_name,
		handle->id);
	
	const struct asset_type_table* p_type_table = &asset_type_tables[CX_ASSET_GET_TYPE_ID(handle->id)];
	if (p_type_table->f_free) {
		p_type_table->f_free(handle->p_asset);
	}

	free(handle->p_asset);
	handle->p_asset = CX_NULL;
}

void* cx_asset_get(cx_asset_handle handle) {
	if (!handle->p_asset) {
		cx_asset_load(handle);
	}
	return handle->p_asset;
}

