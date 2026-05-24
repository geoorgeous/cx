#ifndef CX_ED_IMPORT_GLTF
#define CX_ED_IMPORT_GLTF

#define CX_LOG_CAT_IMPORT_GLTF "import:gltf"

struct cx_asset_package;
struct cx_asset_package_record;
struct gltf;

int cx_ed_import_gltf(
	struct cx_asset_package* p_package,
	const struct gltf* p_gltf,
	struct cx_asset_package_record** pp_out);

int cx_ed_import_gltf_file(
	struct cx_asset_package* p_package,
	const char* s_filepath,
	struct cx_asset_package_record** pp_out);

#endif
