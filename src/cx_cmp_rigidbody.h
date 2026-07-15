#ifndef CX_CMP_RIGIDBODY_H
#define CX_CMP_RIGIDBODY_H

#include "cx_component.h"

extern struct cx_component_type cmp_type_rigidbody;

struct physics_rigidbody;

struct cx_cmp_rigidbody {
	struct physics_rigidbody* p_rigidbody;
};

int cx_cmp_rigidbody_serialize(const void* p_cmp, struct cx_stream* p_stream);
int cx_cmp_rigidbody_deserialize(struct cx_stream* p_stream, void* p_out_cmp);

#endif
