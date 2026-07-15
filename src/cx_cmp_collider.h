#ifndef CX_CMP_COLLIDER_H
#define CX_CMP_COLLIDER_H

#include "cx_component.h"

extern struct cx_component_type cmp_type_collider;

struct physics_object;

struct cx_cmp_collider {
	struct physics_object* p_object;
};

int cx_cmp_collider_serialize(const void* p_cmp, struct cx_stream* p_stream);
int cx_cmp_collider_deserialize(struct cx_stream* p_stream, void* p_out_cmp);

#endif
