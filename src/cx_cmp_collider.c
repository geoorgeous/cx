#include "cx_cmp_collider.h"

struct cx_component_type cmp_type_collider = {
	.s_name = "collider",
	.f_serialize = cx_cmp_collider_serialize,
	.f_deserialize = cx_cmp_collider_deserialize,
	.size = sizeof(struct cx_cmp_collider),
	.alignment = CX_ALIGNOF(struct cx_cmp_collider)
};

int cx_cmp_collider_serialize(const void* p_cmp, struct cx_stream* p_stream) {
	const struct cx_cmp_collider* p_cmp_collider = p_cmp;
	return CX_TRUE;
}

int cx_cmp_collider_deserialize(struct cx_stream* p_stream, void* p_out_cmp) {
	struct cx_cmp_collider* p_out_cmp_collider = p_out_cmp;
	return CX_TRUE;
}
