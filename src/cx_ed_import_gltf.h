#ifndef CX_ED_IMPORT_GLTF
#define CX_ED_IMPORT_GLTF

#define CX_LOG_CAT_IMPORT_GLTF "import:gltf"

#include "cx_asset_types.h"

struct gltf;

int cx_ed_import_gltf(const char* s_name, const struct gltf* p_gltf, struct cx_asset_ref* p_out);
int cx_ed_import_gltf_file(const char* s_filepath, struct cx_asset_ref* p_out);

#endif
