#ifndef CX_IMPORT_BDF_H
#define CX_IMPORT_BDF_H

struct cx_asset_package;
struct cx_asset_package_record;
struct cx_bdf;
struct cx_font;

#define CX_LOG_CAT_IMPORT_BDF "import:bdf"

int cx_ed_import_bdf(
	struct cx_asset_package* p_package,
	const struct cx_bdf* p_bdf,
	struct cx_asset_package_record** pp_out);

int cx_ed_import_bdf_file(
	struct cx_asset_package* p_package,
	const char* s_filepath,
	struct cx_asset_package_record** pp_out);

#endif
