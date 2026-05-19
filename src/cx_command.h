#ifndef CX_COMMAND_H
#define CX_COMMAND_H

#include <stddef.h>
#include <stdint.h>

#define CX_LOG_CAT_COMMAND "command"

#define CX_COMMAND_RESULT_OK 0
#define CX_COMMAND_RESULT_NOEXEC -1

#define CX_COMMAND_MAX_PARAMS 16

#define CX_COMMAND_PARAM_BOOL(NAME, DESC, B_REQUIRED) ((struct cx_command_param){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_COMMAND_PARAM_TYPE_bool,\
	.b_required = B_REQUIRED,\
})

#define CX_COMMAND_PARAM_INT(NAME, DESC, B_REQUIRED) CX_COMMAND_PARAM_INT_RANGE(NAME, DESC, B_REQUIRED, 0, 0)
#define CX_COMMAND_PARAM_INT_RANGE(NAME, DESC, B_REQUIRED, MIN, MAX) ((struct cx_command_param){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_COMMAND_PARAM_TYPE_int,\
	.b_required = B_REQUIRED,\
	.numeric_constraints = { .min = MIN, .max = MAX }\
})

#define CX_COMMAND_PARAM_FLOAT(NAME, DESC, B_REQUIRED) CX_COMMAND_PARAM_FLOAT_RANGE(NAME, DESC, B_REQUIRED, 0, 0)
#define CX_COMMAND_PARAM_FLOAT_RANGE(NAME, DESC, B_REQUIRED, MIN, MAX) ((struct cx_command_param){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_COMMAND_PARAM_TYPE_float,\
	.b_required = B_REQUIRED,\
	.numeric_constraints = { .min = MIN, .max = MAX }\
})

#define CX_COMMAND_PARAM_STRING(NAME, DESC, B_REQUIRED) CX_COMMAND_PARAM_STRING_CANDIDATES(NAME, DESC, B_REQUIRED, 0)
#define CX_COMMAND_PARAM_STRING_CANDIDATES(NAME, DESC, B_REQUIRED, F_CANDIDATE_PROVIDER) ((struct cx_command_param){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_COMMAND_PARAM_TYPE_string,\
	.b_required = B_REQUIRED,\
	.autocomplete = {\
		.provider_type = CX_COMMAND_AUTOCOMPLETE_PROVIDER_TYPE_dynamic,\
		.f_dynamic_provider = F_CANDIDATE_PROVIDER\
	}\
})

#define CX_COMMAND_PARAM_ENUM(NAME, DESC, B_REQUIRED, VALUES, NUM_VALUES) ((struct cx_command_param){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.type = CX_COMMAND_PARAM_TYPE_string,\
	.b_required = B_REQUIRED,\
	.autocomplete = {\
		.provider_type = CX_COMMAND_AUTOCOMPLETE_PROVIDER_TYPE_dynamic,\
		.static_candidates = { .p_s_candidates = VALUES, .num_candidates = NUM_VALUES }\
	}\
})

#define CX_CONSOLE_COMMAND(NAME, DESC, PARAMS, NUM_PARAMS, F) ((struct cx_command){\
	.s_name = NAME,\
	.s_desc = DESC,\
	.p_params = PARAMS,\
	.num_params = NUM_PARAMS,\
	.f = F\
})

enum cx_command_param_type {
	CX_COMMAND_PARAM_TYPE_bool,
	CX_COMMAND_PARAM_TYPE_int,
	CX_COMMAND_PARAM_TYPE_float,
	CX_COMMAND_PARAM_TYPE_string
};

struct cx_command_autocomplete_candidates {
	const char** p_s_candidates;
	size_t num_candidates;
};

typedef void (*cx_command_autocomplete_candidate_provider_fn)(struct cx_command_autocomplete_candidates* p_out_candidates);

enum cx_command_autocomplete_provider_type {
	CX_COMMAND_AUTOCOMPLETE_PROVIDER_TYPE_none,
	CX_COMMAND_AUTOCOMPLETE_PROVIDER_TYPE_static,
	CX_COMMAND_AUTOCOMPLETE_PROVIDER_TYPE_dynamic,
};

struct cx_command_param {
	const char* s_name;
	const char* s_desc;
	enum cx_command_param_type type;
	int b_required;
	
	union {
		struct {
			enum cx_command_autocomplete_provider_type provider_type;
			union {
				struct cx_command_autocomplete_candidates static_provider;
				cx_command_autocomplete_candidate_provider_fn f_dynamic_provider;
			};
		} autocomplete;
		struct {
			double min;
			double max;
		} numeric_constraints;
	};
};

union cx_command_arg {
	int64_t as_int;
	double as_float;
	const char* s_as_str;
	int b_as_bool;
};

struct cx_flogger;

typedef int(*cx_command_fn)(size_t argc, const union cx_command_arg* argv, struct cx_flogger* p_flogger);

struct cx_command {
	const char* s_name;
	const char* s_desc;
	const struct cx_command_param* p_params;
	size_t num_params;
	cx_command_fn f;
};

struct cx_command_registry {
	const struct cx_command** p_commands_;
	size_t commands_cap_;
	size_t num_commands_;
};

void cx_command_registry_free(struct cx_command_registry* p_registry);
void cx_command_registry_add(
	struct cx_command_registry* p_registry,
	const struct cx_command* p_command);
void cx_command_registry_remove(struct cx_command_registry* p_registry, const char* s_command_name);
int cx_command_registry_find(
	const struct cx_command_registry* p_registry,
	const char* s_command_name,
	const struct cx_command** pp_out_command);
int cx_command_registry_execute(
	const struct cx_command_registry* p_registry,
	const char* s_command,
	struct cx_flogger* p_flogger);

int cx_command_resolve_args(
	const struct cx_command* p_command,
	const char* s_args,
	size_t args_len,
	size_t args_max_count,
	union cx_command_arg* p_out_args,
	size_t* p_out_argc);

enum cx_command_parse_arg_result {
	CX_COMMAND_PARSE_ARG_RESULT_success,
	CX_COMMAND_PARSE_ARG_RESULT_expected_bool,
	CX_COMMAND_PARSE_ARG_RESULT_expected_int,
	CX_COMMAND_PARSE_ARG_RESULT_expected_float,
	CX_COMMAND_PARSE_ARG_RESULT_out_of_range
};

enum cx_command_parse_arg_result cx_command_parse_arg(
	const struct cx_command_param* p_param,
	const char* s_arg,
	size_t arg_len,
	union cx_command_arg* p_out_arg);

const char* cx_command_parse_arg_errstr(enum cx_command_parse_arg_result result);

#endif
