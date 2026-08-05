#ifndef CX_ED_IMPORT_H
#define CX_ED_IMPORT_H

#define CX_LOG_CAT_IMPORT "import"

struct cx_asset_ref;

int cx_ed_import_file(const char* s_filepath, struct cx_asset_ref* p_out_asset_ref);

#endif
