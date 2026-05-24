#ifndef CX_CMP_COLLIDER_H
#define CX_CMP_COLLIDER_H

#include "cx_component.h"

CX_COMPONENT_TYPE_DECL(collider);

struct physics_object;

struct cx_cmp_collider {
	struct physics_object* p_object;
};

#endif
