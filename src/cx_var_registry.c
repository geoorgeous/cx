#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cx_logging.h"
#include "cx_macro.h"
#include "cx_var.h"
#include "cx_var_registry.h"

#define CX_VAR_REGISTRY_MAX_NAME_LEN 48

#define CX_VAR_CMP_KEY(P_VAR, KEY) (strcmp((P_VAR)->desc.s_name, KEY))

void cx_var_registry_add(struct cx_var_registry* p_registry, const struct cx_var* p_var) {
	if (!p_var->desc.s_name || p_var->desc.s_name[0] == '\0') {
		CX_LOG(INFO, VAR, "Failed to register var: missing name\n");
		return;
	}

	if (strlen(p_var->desc.s_name) > CX_VAR_REGISTRY_MAX_NAME_LEN) {
		CX_LOG(INFO, VAR, "Failed to register var: name too long\n");
		return;
	}

	for (const char* p = p_var->desc.s_name; *p; p++) {
		if (isalnum(*p) || *p == '_' || *p == '-' || *p == '.') {
			continue;
		}
		CX_LOG_FMT(INFO, VAR,
			"Failed to register var '%s':"
			"name may only contain letters, numbers, underscores, hypens, and periods\n",
			p_var->desc.s_name);
		return;
	}

	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->p_vars,
		p_registry->num_vars,
		p_var->desc.s_name,
		CX_VAR_CMP_KEY,
		&index, &b_found);

	if (b_found) {
		CX_LOG_FMT(INFO, VAR, "Failed to register var '%s': name already taken\n", p_var->desc.s_name);
		return;
	}

	CX_SORTED_ADD(p_registry->p_vars, &p_registry->num_vars, &p_registry->cap_vars, index, p_var);

	CX_LOG_FMT(INFO, VAR, "New var registered: %s\n", p_registry->p_vars[index]->desc.s_name);
}

void cx_var_registry_remove(struct cx_var_registry* p_registry, const char* s_var_name) {
	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->p_vars,
		p_registry->num_vars,
		s_var_name,
		CX_VAR_CMP_KEY,
		&index, &b_found);

	if (!b_found) {
		return;
	}

	CX_SORTED_REMOVE(p_registry->p_vars, &p_registry->num_vars, index);

	// todo: shrink buffer?
}

int cx_var_registry_find(
	const struct cx_var_registry* p_registry,
	const char* s_var_name,
	const struct cx_var** pp_out_var) {
	
	size_t index;
	int b_found;

	CX_BSEARCH(
		p_registry->p_vars,
		p_registry->num_vars,
		s_var_name,
		CX_VAR_CMP_KEY,
		&index, &b_found);

	*pp_out_var = b_found ? p_registry->p_vars[index] : 0;
	return b_found;
}
