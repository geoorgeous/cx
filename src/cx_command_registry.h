#ifndef CX_COMMAND_REGISTRY_H
#define CX_COMMAND_REGISTRY_H

#include <stddef.h>

struct cx_command;

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

struct cx_flogger;

int cx_command_registry_execute(
	const struct cx_command_registry* p_registry,
	const char* s_command,
	struct cx_flogger* p_flogger);

void cx_command_registry_alias(struct cx_command_registry* p_registry, const char* s_alias, const char* s_command);

#endif
