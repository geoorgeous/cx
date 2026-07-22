#ifndef CX_IMPORT_IMAGE_H
#define CX_IMPORT_IMAGE_H

#include <stddef.h>
#include <stdint.h>

#include "cx_asset_types.h"

#define CX_LOG_CAT_IMPORT_IMAGE "import:image"

int cx_ed_import_image(
	const uint8_t* p_bytes,
	size_t size,
	cx_asset_handle* p_out_handle);
int cx_ed_import_image_file(
	const char* s_filepath,
	cx_asset_handle* p_out_handle);

#endif
