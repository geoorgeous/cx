#ifndef CX_CMP_RIGIDBODY_H
#define CX_CMP_RIGIDBODY_H

#include "cx_component.h"

CX_COMPONENT_TYPE_DECL(rigidbody);

struct physics_rigidbody;

struct cx_cmp_rigidbody {
	struct physics_rigidbody* p_rigidbody;
};

#endif
