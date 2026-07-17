#ifndef ASSET_H
#define ASSET_H

#include <stddef.h>
#include <stdint.h>

#define CX_LOG_CAT_ASSET "asset"

#define CX_ASSET_TYPE_ID_MAX 0xFF
#define CX_ASSET_IDN_MAX 0x00FFFFFF
#define CX_ASSET_IDN_MASK CX_ASSET_IDN_MAX

#define CX_ASSET_MAKE_ID(TYPE, IDN) ((uint32_t)(((uint8_t)(TYPE)) << 24) | (((int)(IDN)) & CX_ASSET_IDN_MASK))
#define CX_ASSET_GET_TYPE_ID(ID) ((uint8_t)((ID) >> 24))
#define CX_ASSET_GET_IDN(ID) (((uint32_t)(ID)) & CX_ASSET_IDN_MASK)

typedef uint32_t cx_asset_id;

struct cx_asset_store;

struct cx_asset_ref {
	cx_asset_id id;
	void* p_asset;
	struct cx_asset_store* p_source;
};

struct cx_stream;

typedef int(*cx_asset_serialize_fn)(const void*, struct cx_stream*);
typedef int(*cx_asset_deserialize_fn)(struct cx_stream*, void*);
typedef void(*cx_asset_free_fn)(void*);

void cx_asset_register_type(
	uint8_t asset_type_id,
	const char* s_display_name,
	size_t size,
	cx_asset_serialize_fn f_serialize,
	cx_asset_deserialize_fn f_deserialize,
	cx_asset_free_fn f_free);

int  cx_asset_load(struct cx_asset_ref* p_record);
void cx_asset_free(struct cx_asset_ref* p_record);

int cx_asset_ref_serialize(const struct cx_asset_ref* p_asset_ref, struct cx_stream* p_stream);
int cx_asset_ref_deserialize(struct cx_stream* p_stream, struct cx_asset_ref* p_out_asset_ref);

#endif
