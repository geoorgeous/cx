#ifndef CX_ED_ASSET_PACKAGE_BUILDER_H
#define CX_ED_ASSET_PACKAGE_BUILDER_H

#include "cx_array.h"
#include "cx_asset_types.h"

struct cx_ed_asset_package_builder {
	struct cx_array assets;
};

void cx_ed_asset_package_builder_add_asset(
	struct cx_ed_asset_package_builder* p_builder, const struct cx_asset_ref* p_asset_ref);

void cx_ed_asset_package_builder_export(const struct cx_ed_asset_package_builder* p_builder, const char* s_filepath);

void cx_ed_asset_package_builder_free(struct cx_ed_asset_package_builder* p_builder);

#endif
