#ifndef CX_ASSET_TYPES_H
#define CX_ASSET_TYPES_H

#include <stdint.h>

#define CX_LOG_CAT_ASSET "asset"

#define CX_ASSET_TYPE_ID_MAX 0xFF
#define CX_ASSET_IDN_MAX 0x00FFFFFF
#define CX_ASSET_IDN_MASK CX_ASSET_IDN_MAX

#define CX_ASSET_GET_TYPE_ID(ID) ((uint8_t)((ID) >> 24))
#define CX_ASSET_GET_IDN(ID) (((uint32_t)(ID)) & CX_ASSET_IDN_MASK)

#define CX_ASSET_NAME_MAX_LEN 255

typedef uint8_t cx_asset_type;

typedef uint32_t cx_asset_id;

struct cx_asset_ref {
	cx_asset_id asset_id;
	void** pp_asset;
};

struct cx_stream;

typedef int(*cx_asset_serialize_fn)(const void*, struct cx_stream*);
typedef int(*cx_asset_deserialize_fn)(struct cx_stream*, void*);
typedef void(*cx_asset_enumerate_dependencies_cb_fn)(cx_asset_id, void*);
typedef void(*cx_asset_enumerate_dependencies_fn)(const void*, cx_asset_enumerate_dependencies_cb_fn, void*);
typedef void(*cx_asset_free_fn)(void*);

#endif
