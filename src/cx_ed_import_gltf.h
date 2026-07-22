#ifndef CX_ED_IMPORT_GLTF
#define CX_ED_IMPORT_GLTF

#define CX_LOG_CAT_IMPORT_GLTF "import:gltf"

#include "cx_asset_types.h"

struct gltf;

int cx_ed_import_gltf(
	const struct gltf* p_gltf,
	cx_asset_handle* p_out_handle);
int cx_ed_import_gltf_file(
	const char* s_filepath,
	cx_asset_handle* p_out_handle);

#endif
