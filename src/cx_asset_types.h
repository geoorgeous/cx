#ifndef CX_ASSET_TYPES_H
#define CX_ASSET_TYPES_H

#include <stdint.h>

#define CX_LOG_CAT_ASSET "asset"

#define CX_ASSET_TYPE_ID_MAX 0xFF
#define CX_ASSET_IDN_MAX 0x00FFFFFF
#define CX_ASSET_IDN_MASK CX_ASSET_IDN_MAX

#define CX_ASSET_GET_TYPE_ID(ID) ((uint8_t)((ID) >> 24))
#define CX_ASSET_GET_IDN(ID) (((uint32_t)(ID)) & CX_ASSET_IDN_MASK)

typedef uint8_t cx_asset_type;

typedef uint32_t cx_asset_id;

struct cx_asset_store_record;

typedef struct cx_asset_store_record* cx_asset_handle;

#endif
