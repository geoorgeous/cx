#ifndef CX_COMPONENT_H
#define CX_COMPONENT_H

#include <stddef.h>
#include <stdint.h>

#include "cx_asset_defs.h"
#include "cx_stream_serialization.h"

#define CX_LOG_CAT_COMPONENT "components"

#define CX_COMPONENT_MAX_TYPES 512

#define CX_COMPONENT_TYPE_NAME_MAX_LEN 64

typedef int(*cx_component_serialize_fn)(const void*, struct cx_stream*);
typedef int(*cx_component_deserialize_fn)(struct cx_stream*, void*);
typedef void(*cx_component_enumerate_asset_dependencies_fn)
	(const void*, cx_asset_enumerate_dependencies_cb_fn f_cb, void*);

struct cx_component_type {
	const char* s_name;
	uint16_t    size;
	uint16_t    alignment;
	uint16_t    runtime_id;
	cx_component_serialize_fn f_serialize;
	cx_component_deserialize_fn f_deserialize;
	cx_component_enumerate_asset_dependencies_fn f_enumerate_asset_dependencies;
};

void cx_component_register_type(struct cx_component_type* p_type);

int cx_component_find_type(const char* s_name, const struct cx_component_type** pp_out);

int cx_component_serialize(
	const void* p_component, const struct cx_component_type* p_type, struct cx_stream* p_stream);

int cx_component_deserialize(
	struct cx_stream* p_stream, const struct cx_component_type** pp_out_type, void* p_out_component);

void cx_component_enumerate_asset_dependencies(
	const struct cx_component_type* p_type,
	const void* p_component,
	cx_asset_enumerate_dependencies_cb_fn f_cb,
	void* p_user_ptr);

#endif
