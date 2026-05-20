#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cx_command.h"
#include "cx_command_registry.h"
#include "cx_logging.h"
#include "cx_macro.h"

#define CX_COMMAND_REGISTRY_MAX_NAME_LEN 48

#define CX_COMMAND_CMP_KEY(P_COMMAND, KEY) (strcmp((P_COMMAND)->s_name, KEY))

#define CX_COMMAND_ALIAS_CMP_KEY(COMMAND_ALIAS, KEY) (strcmp((COMMAND_ALIAS).s_name, KEY))

static int cx_command_registry_add_validate_name(const char* s_name);

void cx_command_registry_free(struct cx_command_registry* p_registry) {
	free(p_registry->pp_commands_);
	*p_registry = (struct cx_command_registry){0};
}

void cx_command_registry_add(
	struct cx_command_registry* p_registry,
	const struct cx_command* p_command) {

	if (!cx_command_registry_add_validate_name(p_command->s_name)) {
		return;
	}

	if (!p_command->f) {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register command '%s': missing callback\n", p_command->s_name);
		return;
	}

	if (p_command->num_params > CX_COMMAND_MAX_PARAMS) {
		CX_LOG_FMT(INFO, COMMAND,
			"Failed to register command '%s':"
			"parameter count (%d) exceeded maximum of " CX_STRINGIFY(CX_COMMAND_MAX_PARAMS),
			p_command->s_name, p_command->num_params);
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
	int b_found;

	CX_BSEARCH(
		p_registry->p_aliases_,
		p_registry->num_aliases_,
		p_command->s_name,
		CX_COMMAND_ALIAS_CMP_KEY,
		&index, &b_found);

	if (b_found) {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register command '%s': name taken by alias\n", p_command->s_name);
		return;
	}

	CX_BSEARCH(
		p_registry->pp_commands_,
		p_registry->num_commands_,
		p_command->s_name,
		CX_COMMAND_CMP_KEY,
		&index, &b_found);

	if (b_found) {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register command '%s': name taken\n", p_command->s_name);
		return;
	}

	CX_SORTED_ADD(p_registry->pp_commands_, &p_registry->num_commands_, &p_registry->commands_cap_, index, p_command);

	CX_LOG_FMT(INFO, COMMAND, "New command registered: %s\n", p_registry->pp_commands_[index]->s_name);
}

void cx_command_registry_remove(struct cx_command_registry* p_registry, const char* s_command_name) {
	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->pp_commands_,
		p_registry->num_commands_,
		s_command_name,
		CX_COMMAND_CMP_KEY,
		&index, &b_found);

	if (!b_found) {
		return;
	}

	CX_SORTED_REMOVE(p_registry->pp_commands_, &p_registry->num_commands_, index);
}

int cx_command_registry_find(
	const struct cx_command_registry* p_registry,
	const char* s_command_name,
	const struct cx_command** pp_out_command) {
	
	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->pp_commands_,
		p_registry->num_commands_,
		s_command_name,
		CX_COMMAND_CMP_KEY,
		&index, &b_found);

	*pp_out_command = b_found ? p_registry->pp_commands_[index] : 0;
	return b_found;
}

void cx_command_registry_add_alias(
	struct cx_command_registry* p_registry,
	const char* s_name,
	const char* s_expansion) {

	if (!cx_command_registry_add_validate_name(s_name)) {
		return;
	}

	if (!s_expansion || s_expansion[0] == '\0') {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register '%s': missing expansion\n", s_name);
	}

	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->pp_commands_,
		p_registry->num_commands_,
		s_name,
		CX_COMMAND_CMP_KEY,
		&index, &b_found);

	if (b_found) {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register '%s': name taken by command\n", s_name);
		return;
	}

	CX_BSEARCH(
		p_registry->p_aliases_,
		p_registry->num_aliases_,
		s_name,
		CX_COMMAND_ALIAS_CMP_KEY,
		&index, &b_found);

	if (b_found) {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register '%s': name taken\n", s_name);
		return;
	}

	CX_SORTED_ADD(
		p_registry->p_aliases_,
		&p_registry->num_aliases_,
		&p_registry->aliases_cap_,
		index,
		(struct cx_command_alias){0});

	p_registry->p_aliases_[index].s_name = strdup(s_name);
	p_registry->p_aliases_[index].s_expansion = strdup(s_expansion);

	CX_LOG_FMT(INFO, COMMAND, "New alias registered: %s\n", p_registry->p_aliases_[index].s_name);
}

void cx_command_registry_remove_alias(struct cx_command_registry* p_registry, const char* s_alias_name) {
	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->p_aliases_,
		p_registry->num_aliases_,
		s_alias_name,
		CX_COMMAND_ALIAS_CMP_KEY,
		&index, &b_found);

	if (!b_found) {
		return;
	}

	free(p_registry->p_aliases_[index].s_name);
	free(p_registry->p_aliases_[index].s_expansion);

	CX_SORTED_REMOVE(p_registry->p_aliases_, &p_registry->num_aliases_, index);
}

int cx_command_registry_find_alias(
	struct cx_command_registry* p_registry,
	const char* s_alias_name,
	const struct cx_command_alias** pp_out_alias) {

	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->p_aliases_,
		p_registry->num_aliases_,
		s_alias_name,
		CX_COMMAND_ALIAS_CMP_KEY,
		&index, &b_found);

	*pp_out_alias = b_found ? &p_registry->p_aliases_[index] : 0;
	return b_found;
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
	int b_found;

	CX_BSEARCH(
		p_registry->p_aliases_,
		p_registry->num_aliases_,
		name_buf,
		CX_COMMAND_ALIAS_CMP_KEY,
		&index, &b_found);

	if (b_found) {
		const struct cx_command_alias* p_alias = &p_registry->p_aliases_[index];
		cx_command_registry_execute(p_registry, p_alias->s_expansion, p_flogger);
		return 0;
	}

	CX_BSEARCH(
		p_registry->pp_commands_,
		p_registry->num_commands_,
		name_buf,
		CX_COMMAND_CMP_KEY,
		&index, &b_found);

	if (!b_found) {
		CX_LOG_FMT(INFO, COMMAND, "command not found: %s\n", name_buf);
		return 0;
	}

	const struct cx_command* p_command = p_registry->pp_commands_[index];
	
	struct cx_command_args args;
	if (!cx_command_resolve_args(p_command, s_args, CX_COMMAND_MAX_PARAMS, &args)) {
		return 0;
	}

	const struct cx_command_context context = {
		.p_command = p_command,
		.p_flogger = p_flogger
	};

	int ret = p_command->f(&args, &context);
	CX_LOG_FMT(INFO, COMMAND, "command \"%s, argc=%d, args=%s\" returned %d\n", p_command->s_name, args.count, s_args, ret);
	return ret;
}

int cx_command_registry_add_validate_name(const char* s_name) {
	if (!s_name || s_name[0] == '\0') {
		CX_LOG(INFO, COMMAND, "Failed to register: missing name\n");
		return 0;
	}

	const char* p;
	for (p = s_name; *p; p++) {
		if (isalnum(*p) || *p == '_' || *p == '-' || *p == '.') {
			continue;
		}
		CX_LOG_FMT(INFO, COMMAND,
			"Failed to register '%s':"
			"name may only contain letters [a-z,A-Z], numbers [0-9], underscores '_', hyphens '-', and periods '.'\n",
			s_name);
		return 0;
	}

	if (p - s_name > CX_COMMAND_REGISTRY_MAX_NAME_LEN) {
		CX_LOG_FMT(INFO, COMMAND, "Failed to register '%s': name too long\n", s_name);
		return 0;
	}

	return 1;
}
