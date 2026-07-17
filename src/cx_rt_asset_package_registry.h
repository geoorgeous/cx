#ifndef CX_RT_ASSET_PACKAGE_REGISTRY_H
#define CX_RT_ASSET_PACKAGE_REGISTRY_H

#include <stdint.h>

struct cx_asset_package;
struct cx_asset_ref;
struct cx_stream;

void cx_rt_asset_package_registry_register(struct cx_asset_package* p_package);
void cx_rt_asset_package_registry_free_all(void);
void cx_rt_asset_package_registry_find_asset(uint32_t asset_id, struct cx_asset_ref* p_out);
void cx_rt_asset_package_registry_open_asset_read_stream(uint32_t asset_id, struct cx_stream* p_out_stream);

#endif
