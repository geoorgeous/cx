#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cx_command.h"
#include "cx_logging.h"

int cx_command_resolve_args(
	const struct cx_command* p_command,
	const char* s_args,
	size_t args_max_count,
	struct cx_command_args* p_out_args) {

	*p_out_args = (struct cx_command_args){0};

	const char* s_arg = 0;
	size_t argc = 0;

	for(const char* p = s_args; p && *p; p++) {
		if (!isspace(*p)) {
			s_arg = p;

			if (argc > p_command->num_params) {
				CX_DBG(CX_LOG(INFO, COMMAND, "usage error: too many arguments\n"));
				return 0;
			}

			if (argc == args_max_count) {
				CX_DBG(CX_LOG(INFO, COMMAND, "internal error: argument buffer too small\n"));
				return 0;
			}

			while (*p && !isspace(*p)) {
				// todo: check for quotes here to preserve spces
				p++;
			}

			if (cx_var_parse(
					&p_command->p_params[argc].desc,
					s_arg, p - s_arg,
					&p_out_args->list[argc]) != CX_VAR_PARSE_RESULT_success) {
				CX_DBG(CX_LOG_FMT(INFO, COMMAND, "parse error: invalid argument %d \"%.*s\" (%s)\n",
					argc + 1,
					p - s_arg, s_arg,
					cx_var_type_str(p_command->p_params[argc].desc.type)));
				return 0;
			}

			++argc;
		}
	}

	if (argc < p_command->num_params && p_command->p_params[argc].b_required) {
		CX_DBG(CX_LOG(INFO, COMMAND, "usage error: missing required arguments\n"));
		return 0;
	}

	p_out_args->count = argc;

	return 1;
}


