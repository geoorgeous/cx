#ifndef CX_VAR_REGISTRY_H
#define CX_VAR_REGISTRY_H

#include <stddef.h>

#define CX_NEW_VAR(P, DESC, READONLY) do {\
	const static struct cx_var var_ = CX_VAR(DESC, P, READONLY);\
	cx_var_registry_add(&cx_console_get()->var_registry, &var_); } while(0)

struct cx_var;

struct cx_var_registry {
	const struct cx_var** p_vars;
	size_t num_vars;
	size_t cap_vars;
};

void cx_var_registry_add(struct cx_var_registry* p_registry, const struct cx_var* p_var);

void cx_var_registry_remove(struct cx_var_registry* p_registry, const char* s_var_name);

int cx_var_registry_find(
	const struct cx_var_registry* p_registry,
	const char* s_var_name,
	const struct cx_var** pp_out_var);

#endif
