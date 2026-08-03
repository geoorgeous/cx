#ifndef CX_ED_ASSET_H
#define CX_ED_ASSET_H

#include <stdlib.h>

#include "cx_asset_types.h"

static inline cx_asset_id cx_ed_asset_generate_id(cx_asset_type type) {
	const uint32_t id_number =
		(
			((uint32_t)rand() << 17) +
		 	((uint32_t)rand() <<  2) +
		 	((uint32_t)rand() >> 13)
		) % CX_ASSET_IDN_MAX;
	return	
		(uint32_t)(((uint8_t)type) << 24) |
		(id_number & CX_ASSET_IDN_MASK);
}

#endif
