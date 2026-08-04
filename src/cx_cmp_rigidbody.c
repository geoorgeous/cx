#include "cx_cmp_rigidbody.h"
#include "cx_macro.h"

struct cx_component_type cmp_type_rigidbody = {
	.s_name = "rigidbody",
	.f_serialize = cx_cmp_rigidbody_serialize,
	.f_deserialize = cx_cmp_rigidbody_deserialize,
	.size = sizeof(struct cx_cmp_rigidbody),
	.alignment = CX_ALIGNOF(struct cx_cmp_rigidbody)
};

int cx_cmp_rigidbody_serialize(const void* p_cmp, struct cx_stream* p_stream) {
	const struct cx_cmp_rigidbody* p_cmp_rigidbody = p_cmp;
	return CX_TRUE;
}

int cx_cmp_rigidbody_deserialize(struct cx_stream* p_stream, void* p_out_cmp) {
	struct cx_cmp_rigidbody* p_out_cmp_rigidbody = p_out_cmp;
	return CX_TRUE;
}
