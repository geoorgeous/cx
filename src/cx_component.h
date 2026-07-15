#ifndef CX_COMPONENT_H
#define CX_COMPONENT_H

#include <stddef.h>
#include <stdint.h>

#include "cx_macro.h"
#include "cx_stream_serialization.h"

#define CX_LOG_CAT_COMPONENT "components"

#define CX_COMPONENT_MAX_TYPES 512

#define CX_COMPONENT_TYPE_NAME_MAX_LEN 64

#define CX_COMPONENT_TYPE_DECL(NAME) extern struct cx_component_type cmp_type_##NAME
#define CX_COMPONENT_TYPE_DEFINE(NAME, TYPE) struct cx_component_type cmp_type_##NAME = {\
	.s_name = #NAME,\
	.size = sizeof(TYPE),\
	.alignment = CX_ALIGNOF(TYPE),\
	.f_serialize = cx_cmp_##NAME##_serialize\
	.f_deserialize = cx_cmp_##NAME##_deserialize\
}

typedef int(*cx_component_serialize_fn)(const void*, struct cx_stream*);
typedef int(*cx_component_deserialize_fn)(struct cx_stream*, void*);

struct cx_component_type {
	const char* s_name;
	uint16_t    size;
	uint16_t    alignment;
	uint16_t    runtime_id;
	cx_component_serialize_fn f_serialize;
	cx_component_deserialize_fn f_deserialize;
};

void cx_component_register(struct cx_component_type* p_type);

int cx_component_find_type(const char* s_name, const struct cx_component_type** pp_out);

int cx_component_serialize(
	const void* p_component, const struct cx_component_type* p_type, struct cx_stream* p_stream);

int cx_component_deserialize(
	struct cx_stream* p_stream, const struct cx_component_type** pp_out_type, void* p_out_component);

#endif
