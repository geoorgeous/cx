#ifndef CX_VAR_H
#define CX_VAR_H

#include <stddef.h>
#include <stdint.h>

#define CX_LOG_CAT_VAR "var"

#define CX_VAR_MUTABILITY_READWRITE 0
#define CX_VAR_MUTABILITY_READONLY 1

#define CX_VAR_DESC(NAME, DESC, TYPE) {\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_VAR_TYPE_##TYPE,

#define CX_VAR_DESC_BOOL(NAME, DESC) CX_VAR_DESC(NAME, DESC, bool) }

#define CX_VAR_DESC_INT(NAME, DESC) CX_VAR_DESC(NAME, DESC, int) }

#define CX_VAR_DESC_INT_RANGE(NAME, DESC, MIN, MAX) CX_VAR_DESC(NAME, DESC, int)\
	.type_metadata.numeric_constraints = { .min = MIN, .max = MAX }\
}

#define CX_VAR_DESC_FLOAT(NAME, DESC) CX_VAR_DESC(NAME, DESC, float)}

#define CX_VAR_DESC_FLOAT_RANGE(NAME, DESC, MIN, MAX) CX_VAR_DESC(NAME, DESC, float)\
	.type_metadata.numeric_constraints = { .min = MIN, .max = MAX }\
}

#define CX_VAR_DESC_STRING(NAME, DESC) CX_VAR_DESC(NAME, DESC, string)\
	.type_metadata.string_candidates.provider_type = CX_VAR_CANDIDATES_PROVIDER_TYPE_none\
}

#define CX_VAR_DESC_STRING_CANDIDATES_LIST(NAME, DESC, P_VALUES, N) CX_VAR_DESC(NAME, DESC, string)\
	.type_metadata.string_candidates = {\
		.provider_type = CX_VAR_CANDIDATES_PROVIDER_TYPE_list,\
		.provider.list = {\
			.p_s_candidates = P_VALUES,\
			.num_candidates = N\
		}\
	}\
}

#define CX_VAR_DESC_STRING_CANDIDATES_CALLBACK(NAME, DESC, F) CX_VAR_DESC(NAME, DESC, string)\
	.string_candidates = {\
		.provider_type = CX_VAR_CANDIDATES_PROVIDER_TYPE_callback,\
		.provider.f = F\
	}\
}

#define CX_VAR_DESC_ENUM(NAME, DESC, P_ENTRIES, N) CX_VAR_DESC(NAME, DESC, enum)\
	.type_metadata.enum_map = {\
		.p_entries = P_ENTRIES,\
		.num_entries = N\
	}\
}

enum cx_var_type {
	CX_VAR_TYPE_bool,
	CX_VAR_TYPE_int,
	CX_VAR_TYPE_float,
	CX_VAR_TYPE_string,
	CX_VAR_TYPE_enum
};

struct cx_var_numeric_constraints {
	double min;
	double max;
};

enum cx_var_candidates_provider_type {
	CX_VAR_CANDIDATES_PROVIDER_TYPE_none,
	CX_VAR_CANDIDATES_PROVIDER_TYPE_list,
	CX_VAR_CANDIDATES_PROVIDER_TYPE_callback,
};

struct cx_var_candidates_list {
	const char** p_s_candidates;
	size_t num_candidates;
};

typedef void (*cx_var_candidate_provider_fn)(struct cx_var_candidates_list* p_out_candidates);

struct cx_var_candidates {
	enum cx_var_candidates_provider_type provider_type;
	union {
		struct cx_var_candidates_list list;
		cx_var_candidate_provider_fn f;
	} provider;
};

struct cx_var_enum_map_entry {
	const char* s_name;
	int value;
};

struct cx_var_enum_map {
	const struct cx_var_enum_map_entry* p_entries;
	size_t num_entries;
};

struct cx_var_desc {
	const char* s_name;
	const char* s_desc;
	enum cx_var_type type;
	union {
		struct cx_var_candidates string_candidates;
		struct cx_var_numeric_constraints numeric_constraints;
		struct cx_var_enum_map enum_map;
	} type_metadata;
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
	struct {
		const char* p;
		size_t len;
	} as_str;
	const struct cx_var_enum_map_entry* p_as_enum;
};

enum cx_var_parse_result {
	CX_VAR_PARSE_RESULT_success,
	CX_VAR_PARSE_RESULT_expected_bool,
	CX_VAR_PARSE_RESULT_expected_int,
	CX_VAR_PARSE_RESULT_expected_float,
	CX_VAR_PARSE_RESULT_out_of_range,
	CX_VAR_PARSE_RESULT_invalid_enum
};

const char* cx_var_type_str(enum cx_var_type type);

enum cx_var_parse_result cx_var_parse(
	const struct cx_var_desc* p_desc,
	const char* s,
	size_t len,
	union cx_var_value* p_out);

const char* cx_var_parse_errstr(enum cx_var_parse_result result);

void cx_var_try_set(const struct cx_var* p_var, const char* p_buf, size_t size);

void cx_var_to_str(const struct cx_var* p_var, char* p_buf, size_t size);

#endif
