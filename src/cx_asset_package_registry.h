#ifndef CX_ASSET_PACKAGE_REGISTRY_H
#define CX_ASSET_PACKAGE_REGISTRY_H

#include <stdint.h>

#include "cx_asset_types.h"

struct cx_asset_package;
struct cx_stream;

void cx_asset_package_registry_mount(struct cx_asset_package* p_package);
void cx_asset_package_registry_free_all(void);
int cx_asset_package_registry_find_asset(cx_asset_id asset_id, cx_asset_handle* p_out);
int cx_asset_package_registry_open_asset_read_stream(cx_asset_id asset_id, struct cx_stream* p_out_stream);

#endif
