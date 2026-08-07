#ifndef CX_IMPORT_BDF_H
#define CX_IMPORT_BDF_H

#include "cx_asset_defs.h"

struct cx_bdf;

#define CX_LOG_CAT_IMPORT_BDF "import:bdf"

int cx_ed_import_bdf(const char* s_name, const struct cx_bdf* p_bdf, struct cx_asset_ref* p_out);
int cx_ed_import_bdf_file(const char* s_filepath, struct cx_asset_ref* p_out);

#endif
