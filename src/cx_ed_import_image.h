#ifndef CX_IMPORT_IMAGE_H
#define CX_IMPORT_IMAGE_H

#include <stddef.h>
#include <stdint.h>

struct cx_asset_package;
struct cx_asset_package_record;
struct cx_image;

#define CX_LOG_CAT_IMPORT_IMAGE "import:image"

int cx_ed_import_image(
	struct cx_asset_package* p_package,
	const uint8_t* p_bytes,
	size_t size,
	struct cx_asset_package_record** pp_out);

int cx_ed_import_image_file(
	struct cx_asset_package* p_package,
	const char* s_filepath,
	struct cx_asset_package_record** pp_out);

#endif
