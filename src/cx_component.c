#include "cx_component.h"
#include "cx_dbg.h"

void cx_component_register(struct cx_component_type* p_type) {
	static uint16_t next_runtime_id = 0;

	CX_ASSERT_MSG(next_runtime_id < CX_COMPONENT_MAX_TYPES, "Exceeded maximum number of component types");
	CX_ASSERT_MSG(p_type->runtime_id == CX_COMPONENT_TYPE_ID_INVALID, "Component type already registered");

	p_type->runtime_id = next_runtime_id;
	next_runtime_id++;
}
