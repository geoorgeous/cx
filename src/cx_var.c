#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cx_logging.h"
#include "cx_str.h"
#include "cx_var.h"
#include "math_utils.h"

const char* cx_var_type_str(enum cx_var_type type) {
	static const char* s[] = {
		"bool",
		"int",
		"float",
		"string",
		"enum"
	};
	return s[type];
}

enum cx_var_parse_result cx_var_parse(
	const struct cx_var_desc* p_var_desc,
	const char* s_arg,
	size_t arg_len,
	union cx_var_value* p_out) {

	switch (p_var_desc->type) {
		case CX_VAR_TYPE_bool: {

			if (cx_strcmp_n("true", s_arg, arg_len) == 0 || cx_strcmp_n("1", s_arg, arg_len) == 0) {
				p_out->b_as_bool = 1;
				break;
			}

			if (cx_strcmp_n("false", s_arg, arg_len) == 0 || cx_strcmp_n("0", s_arg, arg_len) == 0) {
				p_out->b_as_bool = 0;
				break;
			}
			
			return CX_VAR_PARSE_RESULT_expected_bool;
		}

#define CX_VAR_NUMBER_IN_RANGE(P_VAR_DESC, X)((\
	!DBL_ISZERO((P_VAR_DESC)->type_metadata.numeric_constraints.min) ||\
	!DBL_ISZERO((P_VAR_DESC)->type_metadata.numeric_constraints.max)) && (\
	(X) < (P_VAR_DESC)->type_metadata.numeric_constraints.min ||\
	(X) > (P_VAR_DESC)->type_metadata.numeric_constraints.max))

		case CX_VAR_TYPE_int: {
			char* p_end;
			const int64_t x = strtoll(s_arg, &p_end, 10);
			
			if ((size_t)(p_end - s_arg) != arg_len) {
				CX_LOG_FMT(INFO, VAR, "Failed to parse string \"%.*s\" to int\n", arg_len, s_arg);
				return CX_VAR_PARSE_RESULT_expected_int;
			}

			if (CX_VAR_NUMBER_IN_RANGE(p_var_desc, (double)x)) {
				CX_LOG_FMT(INFO, VAR, "Value %d (int) outside of range [%g...%g]\n",
					x,
					p_var_desc->type_metadata.numeric_constraints.min,
					p_var_desc->type_metadata.numeric_constraints.max);
				return CX_VAR_PARSE_RESULT_out_of_range;
			}

			p_out->as_int = x;
			break;
		}

		case CX_VAR_TYPE_float: {
			char* p_end;
			const double x = strtod(s_arg, &p_end);
			
			if ((size_t)(p_end - s_arg) != arg_len) {
				CX_LOG_FMT(INFO, VAR, "Failed to parse string \"%.*s\" to float\n", arg_len, s_arg);
				return CX_VAR_PARSE_RESULT_expected_float;
			}

			if (CX_VAR_NUMBER_IN_RANGE(p_var_desc, x)) {
				CX_LOG_FMT(INFO, VAR, "Value %g (float) outside of range [%g...%g]\n",
					x,
					p_var_desc->type_metadata.numeric_constraints.min,
					p_var_desc->type_metadata.numeric_constraints.max);
				return CX_VAR_PARSE_RESULT_out_of_range;
			}

			p_out->as_float = x;
			break;
		}

#undef CX_VAR_NUMBER_IN_RANGE

		case CX_VAR_TYPE_string: {
			p_out->as_str.p = s_arg;
			p_out->as_str.len = arg_len;
			break;
		}

		case CX_VAR_TYPE_enum: {
			char* p_end;
			const int e = (int)strtol(s_arg, &p_end, 10);
			const int b_is_int = (size_t)(p_end - s_arg) == arg_len;

			int b_found_match = 0;
			for (size_t i = 0; i < p_var_desc->type_metadata.enum_map.num_entries; ++i) {
				const struct cx_var_enum_map_entry* p_e = &p_var_desc->type_metadata.enum_map.p_entries[i];
				if ((b_is_int && e == p_e->value) || cx_strcmp_n(p_e->s_name, s_arg, arg_len) == 0) {
					p_out->p_as_enum = p_e;;
					b_found_match = 1;
					break;
				}
			}
			if (!b_found_match) {
				return CX_VAR_PARSE_RESULT_invalid_enum;
			}
			break;
		}
	}
	
	return CX_VAR_PARSE_RESULT_success;
}

const char* cx_var_parse_errstr(enum cx_var_parse_result result) {
	static const char* strs[] = {
		0,
		"Expected boolean value",
		"Expected integer value",
		"Expected float value",
		"Value outside of expected range",
		"Invalid enum value"
	};
	return strs[result];
}

void cx_var_try_set(const struct cx_var* p_var, const char* p_buf, size_t size) {
	if (p_var->b_readonly) {
		return;
	}

	union cx_var_value val;
	const enum cx_var_parse_result r = cx_var_parse(&p_var->desc, p_buf, size, &val);

	if (r != CX_VAR_PARSE_RESULT_success) {
		return;
	}

	switch(p_var->desc.type) {
		case CX_VAR_TYPE_string: {
			strncpy(p_var->p, val.as_str.p, val.as_str.len);
			break;
		}
		case CX_VAR_TYPE_float: {
			*((double*)p_var->p) = val.as_float;
			break;
		}
		case CX_VAR_TYPE_int: {
			*((int64_t*)p_var->p) = val.as_int;
			break;
		}
		case CX_VAR_TYPE_bool: {
			*((int*)p_var->p) = val.b_as_bool;
			break;
		}
		case CX_VAR_TYPE_enum: {
			*((int*)p_var->p) = val.p_as_enum->value;
			break;
		}
	}
}

void cx_var_to_str(const struct cx_var* p_var, char* p_buf, size_t size) {
	switch(p_var->desc.type) {
		case CX_VAR_TYPE_string: {
			strncpy(p_buf, p_var->p, size - 1);
			break;
		}
		case CX_VAR_TYPE_float: {
			const double* p_d = p_var->p;
			snprintf(p_buf, size - 1, "%g", *p_d);
			break;
		}
		case CX_VAR_TYPE_int: {
			const int64_t* p_i = p_var->p;
			snprintf(p_buf, size - 1, "%"PRId64, *p_i);
			break;
		}
		case CX_VAR_TYPE_bool: {
			const int* p_b = p_var->p;
			strncpy(p_buf, *p_b ? "true" : "false", size - 1);
			break;
		}
		case CX_VAR_TYPE_enum: {
			const int* p_e = p_var->p;
			for (size_t i = 0; i < p_var->desc.type_metadata.enum_map.num_entries; ++i) {
				if (*p_e == p_var->desc.type_metadata.enum_map.p_entries[i].value) {
					strncpy(p_buf, p_var->desc.type_metadata.enum_map.p_entries[i].s_name, size - 1);
					return;
				}
			}
			strncpy(p_buf, "???", size - 1);
			break;
		}
	}
}
