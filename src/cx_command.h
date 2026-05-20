#ifndef CX_COMMAND_H
#define CX_COMMAND_H

#include <stddef.h>
#include <stdint.h>

#include "cx_var.h"

#define CX_LOG_CAT_COMMAND "command"

#define CX_COMMAND_MAX_PARAMS 16

#define CX_COMMAND_PARAM_REQUIRED 1
#define CX_COMMAND_PARAM_OPTIONAL 0

#define CX_COMMAND_PARAM(DESC, REQUIRED) ((struct cx_command_param){\
	.desc = CX_VAR_DESC_##DESC,\
	.b_required = CX_COMMAND_PARAM_##REQUIRED,\
})

struct cx_command_param {
	struct cx_var_desc desc;
	int b_required;
};

struct cx_command_args {
	union cx_var_value list[CX_COMMAND_MAX_PARAMS];
	size_t count;
};

struct cx_flogger;

struct cx_command_context {
	const struct cx_command* p_command;
	struct cx_flogger* p_flogger;
};

typedef int(*cx_command_fn)(const struct cx_command_args* p_args, const struct cx_command_context* p_context);

struct cx_command {
	const char* s_name;
	const char* s_desc;
	const struct cx_command_param* p_params;
	size_t num_params;
	cx_command_fn f;
	void* p_user_ptr;
};

int cx_command_resolve_args(
	const struct cx_command* p_command,
	const char* s_args,
	size_t args_max_count,
	struct cx_command_args* p_out_args);

#endif
