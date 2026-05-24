#ifndef CX_CMP_STATIC_MESH_H
#define CX_CMP_STATIC_MESH_H

#include "cx_component.h"

CX_COMPONENT_TYPE_DECL(static_mesh);

struct cx_asset_package_record;

struct cx_cmp_static_mesh {
	struct cx_asset_package_record* p_asset_package_record;
};

#endif
