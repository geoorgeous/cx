#include <string.h>
#include <stdlib.h>

#include "cx_var.h"
#include "math_utils.h"

enum cx_var_parse_result cx_var_parse(
	const struct cx_var_desc* p_var_desc,
	const char* s_arg,
	size_t arg_len,
	union cx_var_value* p_out) {

	switch (p_var_desc->type) {
		case CX_VAR_TYPE_bool: {
			if (strncmp("true", s_arg, arg_len) == 0 || strncmp("1", s_arg, arg_len) == 0) {
				p_out->b_as_bool = 1;
				break;
			}

			if (strncmp("false", s_arg, arg_len) == 0 || strncmp("0", s_arg, arg_len) == 0) {
				p_out->b_as_bool = 0;
				break;
			}
			
			return CX_VAR_PARSE_RESULT_expected_bool;
		}

		case CX_VAR_TYPE_int: {
			char* p_end;
			const uint64_t x = (int64_t)strtoll(s_arg, &p_end, 10);
			if (s_arg != p_end && *p_end != '\0') {
				return CX_VAR_PARSE_RESULT_expected_int;
			}

			if ((FLT_ISZERO(p_var_desc->numeric_constraints.min) && FLT_ISZERO(p_var_desc->numeric_constraints.max)) ||
				(x >= p_var_desc->numeric_constraints.min && x <= p_var_desc->numeric_constraints.max)) {
				return CX_VAR_PARSE_RESULT_out_of_range;
			}

			p_out->as_int = x;
			break;
		}

		case CX_VAR_TYPE_float: {
			char* p_end;
			const double x = strtod(s_arg, &p_end);
			if (s_arg != p_end && *p_end != '\0') {
				return CX_VAR_PARSE_RESULT_expected_float;
			}

			if ((FLT_ISZERO(p_var_desc->numeric_constraints.min) && FLT_ISZERO(p_var_desc->numeric_constraints.max)) ||
				(x >= p_var_desc->numeric_constraints.min && x <= p_var_desc->numeric_constraints.max)) {
				return CX_VAR_PARSE_RESULT_out_of_range;
			}

			p_out->as_float = x;
			break;
		}

		case CX_VAR_TYPE_string: {
			p_out->s_as_str = s_arg;
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
		"Value outside of expected range"
	};
	return strs[result];
}
