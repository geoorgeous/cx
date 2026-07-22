#ifndef CX_ED_ASSET_PACKAGE_BUILDER_H
#define CX_ED_ASSET_PACKAGE_BUILDER_H

#include "cx_asset_types.h"

struct cx_asset_package;

void cx_ed_asset_package_builder_add_asset(struct cx_asset_package* p_package, cx_asset_handle asset);

void cx_ed_asset_package_builder_export(const struct cx_asset_package* p_package, const char* s_filepath);

#endif
