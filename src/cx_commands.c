#include <stdlib.h>
#include <string.h>

#include "cx_commands.h"
#include "cx_logging.h"
#include "hashtable.h"

struct cx_command {
	struct cx_command_info info;
	cx_command_proc        f_proc;
};

static struct hashtable commands;

static int  cx_commands_init(void);
static void cx_commands_help(const struct cx_command_context* p_context);

void cx_commands_register(const struct cx_command_info* p_command_info, cx_command_proc f_proc) {
	if (!cx_commands_init()) {
		return;
	}

 	struct cx_command* p_command = hashtable_s_add(&commands, p_command_info->s_id);

	struct hashtable_itr itr;
	hashtable_s_find(&commands, p_command_info->s_id, &itr);

	*p_command = (struct cx_command) {
		.info = {
			.s_id = itr.p_key,
			.s_description = strdup(p_command_info->s_description),
			.p_data_ptr = p_command_info->p_data_ptr
		},
		.f_proc = f_proc
	};
}

void cx_commands_unregister(const char* s_command) {
	if (!cx_commands_init()) {
		return;
	}

	struct hashtable_itr itr;
	if (!hashtable_s_find(&commands, s_command, &itr)) {
		return;
	}

	struct cx_command* p_command = itr.p_value;
	free(p_command->info.s_description);

	hashtable_s_remove(&commands, s_command);
}

void cx_commands_execute(const char* s_command, int argc, const char* argv[]) {
	if (!cx_commands_init()) {
		return;
	}

CX_DBG(
	CX_LOG_FMT(INFO, COMMANDS, "Executing command '%s' with %d argument(s)\n", s_command, argc);
	if (argc > 0) {
		CX_LOG(INFO, COMMANDS, "  Arguments:\n");
		for (int i = 0; i < argc; ++i) {
			CX_LOG_FMT(INFO, COMMANDS, "  [%d] %s\n", i, argv[i]);
		}
	}
);

	struct hashtable_itr itr;
	if (!hashtable_s_find(&commands, s_command, &itr)) {
		CX_LOG_FMT(WARNING, COMMANDS, "No command '%s'\n", s_command);
		return;
	}

	const struct cx_command* p_command = itr.p_value;
	p_command->f_proc(&(struct cx_command_context){
		.p_command = &p_command->info,
		.argc = argc,
		.s_args = argv
	});
}

int cx_commands_init(void) {
	if (commands.element_size_ != 0) {
		return 1;
	}

	hashtable_init(&commands, sizeof(struct cx_command));

	cx_commands_register(&(struct cx_command_info){
		.s_id = "help",
		.s_description = "List information about all registered commands.",
	}, cx_commands_help);

	return 1;
}

void cx_commands_help(const struct cx_command_context* p_context) {
	(void)p_context;

	struct hashtable_itr itr;
	hashtable_itr(&commands, &itr);

	CX_LOG_FMT(INFO, COMMANDS, "Registered commands (%d):\n", commands.n_elements_);
	while (hashtable_itr_is_valid(&itr)) {
		const struct cx_command* p_command = itr.p_value;
		CX_LOG_FMT(INFO, COMMANDS, "  %s -- %s\n", p_command->info.s_id, p_command->info.s_description);
		hashtable_itr_next(&itr);
	}
}
