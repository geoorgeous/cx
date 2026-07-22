#ifndef CX_IMPORT_BDF_H
#define CX_IMPORT_BDF_H

#include "cx_asset_types.h"

struct cx_bdf;

#define CX_LOG_CAT_IMPORT_BDF "import:bdf"

int cx_ed_import_bdf(const struct cx_bdf* p_bdf, cx_asset_handle* p_out_handle);
int cx_ed_import_bdf_file(const char* s_filepath, cx_asset_handle* p_out_handle);

#endif
