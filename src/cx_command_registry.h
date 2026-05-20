#ifndef CX_COMMAND_REGISTRY_H
#define CX_COMMAND_REGISTRY_H

#include <stddef.h>

#define CX_NEW_COMMAND(NAME, DESC, F, ...) do {\
	const static struct cx_command_param NAME##_command_params[] = {\
		__VA_ARGS__\
	};\
	const static struct cx_command NAME##_command = {\
		.s_name = #NAME,\
		.s_desc = DESC,\
		.p_params = NAME##_command_params,\
		.num_params = sizeof(NAME##_command_params) / sizeof(struct cx_command_param),\
		.f = F\
	};\
	cx_command_registry_add(&cx_console_get()->command_registry, &NAME##_command); } while(0)

struct cx_command;

struct cx_command_alias {
	const char* s_name;
	const char* s_expansion;
};

struct cx_command_registry {
	const struct cx_command** p_commands_;
	size_t commands_cap_;
	size_t num_commands_;
	const struct cx_command_alias** p_aliases_;
	size_t aliases_cap_;
	size_t num_aliases_;
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

void cx_command_registry_add_alias(struct cx_command_registry* p_registry, const struct cx_command_alias* p_alias);

void cx_command_registry_remove_alias(struct cx_command_registry* p_registry, const char* s_alias_name);

int cx_command_registry_find_alias(
	struct cx_command_registry* p_registry,
	const char* s_alias_name,
	const struct cx_command_alias** pp_out_alias);

struct cx_flogger;

int cx_command_registry_execute(
	const struct cx_command_registry* p_registry,
	const char* s_command,
	struct cx_flogger* p_flogger);

#endif
