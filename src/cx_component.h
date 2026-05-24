#ifndef CX_COMPONENT_H
#define CX_COMPONENT_H

#include <stddef.h>
#include <stdint.h>

#define CX_COMPONENT_MAX_TYPES 512

#define CX_COMPONENT_TYPE_ID_INVALID UINT16_MAX

#define CX_COMPONENT_TYPE_DECL(NAME) extern struct cx_component_type cmp_type_##NAME
#define CX_COMPONENT_TYPE_DEFINE(NAME, TYPE) struct cx_component_type cmp_type_##NAME = {\
	.s_name = #NAME,\
	.size = sizeof(TYPE),\
	.runtime_id = CX_COMPONENT_TYPE_ID_INVALID\
};

struct cx_component_type {
	const char* s_name;
	uint16_t    size;
	uint16_t    runtime_id;
};

void cx_component_register(struct cx_component_type* p_type);

#endif
