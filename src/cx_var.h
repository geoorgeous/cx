#ifndef CX_VAR_H
#define CX_VAR_H

#include <stddef.h>
#include <stdint.h>

#define CX_VAR_MUTABILITY_READWRITE 0
#define CX_VAR_MUTABILITY_READONLY 1

#define CX_VAR_DESC_BOOL(NAME, DESC) ((struct cx_var_desc){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_VAR_TYPE_bool,\
})

#define CX_VAR_DESC_INT(NAME, DESC) CX_VAR_DESC_INT_RANGE(NAME, DESC, 0, 0)
#define CX_VAR_DESC_INT_RANGE(NAME, DESC, MIN, MAX) ((struct cx_var_desc){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_VAR_TYPE_int,\
	.numeric_constraints = { .min = MIN, .max = MAX }\
})

#define CX_VAR_DESC_FLOAT(NAME, DESC) CX_VAR_DESC_FLOAT_RANGE(NAME, DESC, 0, 0)
#define CX_VAR_DESC_FLOAT_RANGE(NAME, DESC, MIN, MAX) ((struct cx_var_desc){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_VAR_TYPE_float,\
	.numeric_constraints = { .min = MIN, .max = MAX }\
})

#define CX_VAR_DESC_STRING(NAME, DESC) CX_VAR_DESC_STRING_CANDIDATES(NAME, DESC, 0)
#define CX_VAR_DESC_STRING_CANDIDATES(NAME, DESC, F_CANDIDATE_PROVIDER) ((struct cx_var_desc){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_VAR_TYPE_string,\
	.candidates = {\
		.provider_type = CX_VAR_CANDIDATES_PROVIDER_TYPE_dynamic,\
		.f_dynamic_provider = F_CANDIDATE_PROVIDER\
	}\
})

#define CX_VAR_DESC_ENUM(NAME, DESC, VALUES, NUM_VALUES) ((struct cx_var_desc){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_VAR_TYPE_string,\
	.candidates = {\
		.provider_type = CX_VAR_CANDIDATES_PROVIDER_TYPE_static,\
		.static_candidates = { .p_s_candidates = VALUES, .num_candidates = NUM_VALUES }\
	}\
})

#define CX_VAR(DESC, P, READONLY) ((struct cx_var){\
	.desc = CX_VAR_DESC_##DESC,\
	.p = P,\
	.b_readonly = CX_VAR_MUTABILITY_##READONLY,\
})

enum cx_var_type {
	CX_VAR_TYPE_bool,
	CX_VAR_TYPE_int,
	CX_VAR_TYPE_float,
	CX_VAR_TYPE_string
};

struct cx_var_candidates {
	const char** p_s_candidates;
	size_t num_candidates;
};

typedef void (*cx_var_candidate_provider_fn)(struct cx_var_candidates* p_out_candidates);

enum cx_var_candidates_provider_type {
	CX_VAR_CANDIDATES_PROVIDER_TYPE_none,
	CX_VAR_CANDIDATES_PROVIDER_TYPE_static,
	CX_VAR_CANDIDATES_PROVIDER_TYPE_dynamic,
};

struct cx_var_desc {
	const char* s_name;
	const char* s_desc;
	enum cx_var_type type;
	union {
		struct {
			enum cx_var_candidates_provider_type provider_type;
			union {
				struct cx_var_candidates static_provider;
				cx_var_candidate_provider_fn f_dynamic_provider;
			};
		} candidates;
		struct {
			double min;
			double max;
		} numeric_constraints;
	};
};

struct cx_var {
	struct cx_var_desc desc;
	void* p;
	int b_readonly;
};

union cx_var_value {
	int b_as_bool;
	int64_t as_int;
	double as_float;
	const char* s_as_str;
};

enum cx_var_parse_result {
	CX_VAR_PARSE_RESULT_success,
	CX_VAR_PARSE_RESULT_expected_bool,
	CX_VAR_PARSE_RESULT_expected_int,
	CX_VAR_PARSE_RESULT_expected_float,
	CX_VAR_PARSE_RESULT_out_of_range
};

enum cx_var_parse_result cx_var_parse(
	const struct cx_var_desc* p_desc,
	const char* s,
	size_t len,
	union cx_var_value* p_out);

const char* cx_var_parse_errstr(enum cx_var_parse_result result);

void cx_var_try_set(const struct cx_var* p_var, const char* p_buf, size_t size);

void cx_var_to_str(const struct cx_var* p_var, char* p_buf, size_t size);

#endif
