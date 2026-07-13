#include "cx_component.h"
#include "cx_dbg.h"

void cx_component_register(struct cx_component_type* p_type) {
	static uint16_t next_runtime_id = 0;

	CX_ASSERT_MSG(next_runtime_id < CX_COMPONENT_MAX_TYPES, COMPONENT, "Exceeded maximum number of component types");
	CX_ASSERT_MSG(p_type->runtime_id == CX_COMPONENT_TYPE_ID_INVALID, COMPONENT, "Component type already registered");

	p_type->runtime_id = next_runtime_id;
	next_runtime_id++;
}

int cx_component_find_type(const char *s_name, const struct cx_component_type **pp_out) {
	return CX_FALSE;
}

int cx_component_serialize(
	const void* p_component, const struct cx_component_type* p_type, struct cx_stream_writer* p_writer) {
	return
		cx_stream_serialize_string(p_writer, p_type->s_name, 0) &&
		p_type->f_serialize(p_component, p_writer);
}

int cx_component_deserialize(
	void* p_component, struct cx_stream_reader* p_reader, const struct cx_component_type** pp_out_type) {
	char component_type_name_buf[CX_COMPONENT_TYPE_NAME_MAX_LEN + 1];
	size_t component_type_name_len;
	
	cx_stream_deserialize_string(p_reader, component_type_name_buf, &component_type_name_len);
	component_type_name_buf[component_type_name_len] = '\0';

	if (!cx_component_find_type(component_type_name_buf, pp_out_type)) {
		return CX_FALSE;
	}

	return (*pp_out_type)->f_deserialize(p_component, p_reader);
}
