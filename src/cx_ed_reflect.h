#ifndef CX_ED_REFLECT_H
#define CX_ED_REFLECT_H

#include <stddef.h>
#include <stdint.h>

#include "cx_str.h"

enum cx_ed_reflect_type {
	CX_ED_REFLECT_TYPE_bool,
	CX_ED_REFLECT_TYPE_uint8,
	CX_ED_REFLECT_TYPE_int8,
	CX_ED_REFLECT_TYPE_uint16,
	CX_ED_REFLECT_TYPE_int16,
	CX_ED_REFLECT_TYPE_uint32,
	CX_ED_REFLECT_TYPE_int32,
	CX_ED_REFLECT_TYPE_float,
	CX_ED_REFLECT_TYPE_double,

	CX_ED_REFLECT_TYPE_enum,

	CX_ED_REFLECT_TYPE_array,

	CX_ED_REFLECT_TYPE_struct
};

struct cx_ed_reflect_field {
	const char* s_name;
	enum cx_ed_reflect_type type;
	size_t offset;
};

struct cx_ed_reflect_struct {
	const char* s_name;
	size_t size;
	const struct cx_ed_reflect_field* p_fields;
	size_t num_fields;
};

inline static int cx_ed_reflect_find_field(
	const struct cx_ed_reflect_struct* p_struct, const char* s_name, const struct cx_ed_reflect_field** p_out_field) {

	for (size_t i = 0; i < p_struct->num_fields; ++i) {
		if (cx_str_eq(s_name, p_struct->p_fields[i].s_name)) {
			*p_out_field = &p_struct->p_fields[i];
			return CX_TRUE;
		}
	}
	return CX_FALSE;
}

inline static void* cx_ed_reflect_field_address(void* p_struct_data, const struct cx_ed_reflect_field* p_field) {
	return (uint8_t*)p_struct_data + p_field->offset;
}

#endif
