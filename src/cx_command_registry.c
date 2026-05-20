#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cx_command.h"
#include "cx_command_registry.h"
#include "cx_logging.h"
#include "cx_macro.h"

#define CX_COMMAND_REGISTRY_INITIAL_SIZE 8
#define CX_COMMAND_REGISTRY_MAX_NAME_LEN 48

int cmp_command(const void* p_a, const void* p_b);
int cx_command_registry_find_index(
	const struct cx_command_registry* p_registry,
	const char* s,
	size_t* p_out_index);

void cx_command_registry_free(struct cx_command_registry* p_registry) {
	free(p_registry->p_commands_);
	*p_registry = (struct cx_command_registry){0};
}

void cx_command_registry_add(
	struct cx_command_registry* p_registry,
	const struct cx_command* p_command) {

	if (!p_command->s_name || p_command->s_name[0] == '\0') {
		CX_LOG(INFO, COMMAND, "Failed to register command: missing name\n");
		return;
	}

	if (strlen(p_command->s_name) > CX_COMMAND_REGISTRY_MAX_NAME_LEN) {
		CX_LOG(INFO, COMMAND, "Failed to register command: name too long\n");
		return;
	}

	if (p_command->num_params > CX_COMMAND_MAX_PARAMS) {
		CX_LOG_FMT(INFO, COMMAND,
			"Failed to register command '%s':"
			"parameter count (%d) exceeded maximum of " CX_STRINGIFY(CX_COMMAND_MAX_PARAMS),
			p_command->s_name, p_command->num_params);
	}

	for (const char* p = p_command->s_name; *p; p++) {
		if (isalnum(*p) || *p == '_' || *p == '-' || *p == '.') {
			continue;
		}
		CX_LOG_FMT(INFO, COMMAND,
			"Failed to register command '%s':"
			"name may only contain letters, numbers, underscores, hypens, and periods\n",
			p_command->s_name);
		return;
	}

	if (!p_command->f) {
		CX_LOG_FMT(INFO, COMMAND,
			"Failed to register command '%s':"
			"missing callback\n",
			p_command->s_name);
		return;
	}

	for (size_t i = 1; i < p_command->num_params; ++i) {
		if (p_command->p_params[i - 1].b_required || !p_command->p_params[i].b_required) {
			continue;
		}
		CX_LOG_FMT(INFO, COMMAND,
			"Failed to register command '%s':"
			"required parameter '%s' found after optional parameter '%s'\n",
			p_command->p_params[i].desc.s_name, p_command->p_params[i - 1].desc.s_name);
		return;
	}

	// todo: make sure param names are valid and unique 

	size_t index;
	if (cx_command_registry_find_index(p_registry, p_command->s_name, &index)) {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register command '%s': name already taken\n", p_command->s_name);
		return;
	}

	if (p_registry->commands_cap_ == p_registry->num_commands_) {
		p_registry->commands_cap_ =
			p_registry->commands_cap_ ?
			p_registry->commands_cap_ * 2 :
			CX_COMMAND_REGISTRY_INITIAL_SIZE;
		const size_t new_size = p_registry->commands_cap_ * sizeof(*p_registry->p_commands_);
		p_registry->p_commands_ = realloc(p_registry->p_commands_, new_size);
	}
	
	if (index < p_registry->num_commands_) {
		void* p_dst = p_registry->p_commands_ + index + 1;
		void* p_src = p_registry->p_commands_ + index;
		size_t size = sizeof(*p_registry->p_commands_) * (p_registry->num_commands_ - index);
		memmove(p_dst, p_src, size);
	}

	++p_registry->num_commands_;
	p_registry->p_commands_[index] = p_command;

	CX_LOG_FMT(INFO, COMMAND, "New command registered: %s\n", p_registry->p_commands_[index]->s_name);
}

void cx_command_registry_remove(struct cx_command_registry* p_registry, const char* s_command_name) {
	size_t index;
	if (!cx_command_registry_find_index(p_registry, s_command_name, &index)) {
		return;
	}

	--p_registry->num_commands_;

	void* p_dst = p_registry->p_commands_ + index;
	void* p_src = p_registry->p_commands_ + index + 1;
	size_t size = sizeof(*p_registry->p_commands_) * (p_registry->num_commands_ - index);
	memmove(p_dst, p_src, size);

	// todo: shrink buffer?
}

int cx_command_registry_find(
	const struct cx_command_registry* p_registry,
	const char* s_command_name,
	const struct cx_command** pp_out_command) {
	
	size_t index;
	const int b_success = cx_command_registry_find_index(p_registry, s_command_name, &index);

	if (!b_success) {
		*pp_out_command = 0;
		return 0;
	}

	*pp_out_command = p_registry->p_commands_[index];
	return 1;
}

int cx_command_registry_execute(
	const struct cx_command_registry* p_registry,
	const char* s_command,
	struct cx_flogger* p_flogger) {

	const char* p = s_command;
	const char* s_command_name = 0;
	const char* s_args = 0;
	for(; *p; p++) {
		if (isspace(*p)) {
			if (s_command_name) {
				s_args = p;
				break;
			}
			continue;
		}
		if (!s_command_name) {
			s_command_name = p;
			continue;
		}
	}

	if (!s_command_name) {
		return 0;
	}

	char name_buf[CX_COMMAND_REGISTRY_MAX_NAME_LEN + 1];
	const size_t name_len = p - s_command_name;
	memcpy(name_buf, s_command_name, name_len);
	name_buf[name_len] = '\0';

	size_t index;
	if (!cx_command_registry_find_index(p_registry, name_buf, &index)) {
		CX_LOG_FMT(INFO, COMMAND, "command not found: %s\n", name_buf);
		return 0;
	}

	const struct cx_command* p_command = p_registry->p_commands_[index];
	
	struct cx_command_args args;
	if (!cx_command_resolve_args(p_command, s_args, CX_COMMAND_MAX_PARAMS, &args)) {
		CX_LOG_FMT(INFO, COMMAND, "bad arguments for command: %s\n", name_buf);
		return 0;
	}

	CX_LOG_FMT(INFO, COMMAND, "executing command '%s' with %d arguments\n", name_buf, args.count);

	const struct cx_command_context context = {
		.p_command = p_command,
		.p_flogger = p_flogger
	};

	return p_command->f(&args, &context);
}

void cx_command_registry_alias(struct cx_command_registry* p_registry, const char* s_alias, const char* s_command) {
	
}

int cmp_command(const void* p_a, const void* p_b) {
	const struct cx_command* const* p_command_a = p_a;
	const struct cx_command* const* p_command_b = p_b;
	return strcmp((*p_command_a)->s_name, (*p_command_b)->s_name);
}

int cx_command_registry_find_index(
	const struct cx_command_registry* p_registry,
	const char* s,
	size_t* p_out_index) {

	size_t lo = 0;
	size_t hi = p_registry->num_commands_;
	while(lo < hi) {
		const size_t mid = lo + (hi - lo) / 2;
		const int cmp = strcmp(p_registry->p_commands_[mid]->s_name, s);
		if (cmp < 0) {
			lo = mid + 1;
		} else if (cmp > 0) {
			hi = mid;
		} else {
			*p_out_index = mid;
			return 1;
		}
	}

	*p_out_index = lo;
	return 0;
}
