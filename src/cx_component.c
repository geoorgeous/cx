#include <string.h>

#include "cx_array.h"
#include "cx_component.h"
#include "cx_logging.h"
#include "cx_macro.h"

struct cx_array component_types;

void cx_component_register_type(struct cx_component_type* p_type) {
	if (component_types.element_size == 0) {
		cx_array_init(sizeof(struct cx_component_type*), &component_types);
	}

	(void)cx_array_push(&component_types, &p_type);
	p_type->runtime_id = (uint16_t)component_types.length;
}

int cx_component_find_type(const char *s_name, const struct cx_component_type **pp_out) {
	for (size_t i = 0; i < component_types.length; ++i) {
		const struct cx_component_type** pp_type = cx_array_at(&component_types, i);
		if (strcmp(s_name, (*pp_type)->s_name) == 0) {
			*pp_out = *pp_type;
			return CX_TRUE;
		}
	}
	CX_LOG_FMT(ERROR, COMPONENT, "Failed to find type with name \"%s\"\n", s_name);
	return CX_FALSE;
}

int cx_component_serialize(
	const void* p_component, const struct cx_component_type* p_type, struct cx_stream* p_stream) {
	return
		cx_stream_serialize_string(p_stream, p_type->s_name, 0) &&
		p_type->f_serialize(p_component, p_stream);
}

int cx_component_deserialize(
	struct cx_stream* p_stream, const struct cx_component_type** pp_out_type, void* p_out_component) {
	char component_type_name_buf[CX_COMPONENT_TYPE_NAME_MAX_LEN + 1];
	
	size_t component_type_name_len;
	cx_stream_deserialize_cstring(p_stream, component_type_name_buf, &component_type_name_len);

	if (!cx_component_find_type(component_type_name_buf, pp_out_type)) {
		return CX_FALSE;
	}

	return (*pp_out_type)->f_deserialize(p_stream, p_out_component);
}

void cx_component_enumerate_asset_dependencies(
	const struct cx_component_type* p_type,
	const void* p_component,
	cx_asset_enumerate_dependencies_cb_fn f_cb,
	void* p_user_ptr) {

	if (p_type->f_enumerate_asset_dependencies) {
		p_type->f_enumerate_asset_dependencies(p_component, f_cb, p_user_ptr);
	}
}
