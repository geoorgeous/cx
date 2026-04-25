#include "cx_commands.h"
#include "cx_logging.h"
#include "hashtable.h"

struct cx_command {
	command_proc f_proc;
};

static struct hashtable commands;

static int  cx_commands_init(void);
static void cx_commands_help(int argc, const char* argv[]);

void cx_commands_register(const char* s_command, command_proc f_proc) {
	if (!cx_commands_init()) {
		return;
	}

 	struct cx_command* p_command = hashtable_s_add(&commands, s_command);

	p_command->f_proc = f_proc;
}

void cx_commands_unregister(const char* s_command) {
	if (!cx_commands_init()) {
		return;
	}

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

	struct cx_command* p_command = hashtable_s_find(&commands, s_command);

	if (!p_command) {
		CX_LOG_FMT(WARNING, COMMANDS, "No command '%s'\n", s_command);
		return;
	}

	p_command->f_proc(argc, argv);
}

int cx_commands_init(void) {
	if (commands._element_size != 0) {
		return 1;
	}

	hashtable_init(&commands, sizeof(struct cx_command));

	cx_commands_register("help", cx_commands_help);

	return 1;
}

void cx_commands_help(int argc, const char* argv[]) {
	(void)argc;
	(void)argv;

	struct hashtable_itr itr;
	hashtable_itr(&commands, &itr);

	CX_LOG_FMT(INFO, COMMANDS, "Registered commands (%d):\n", commands._n_elements);
	while (hashtable_itr_is_valid(&itr)) {
		CX_LOG_FMT(INFO, COMMANDS, "  %s\n", itr.p_key);
		hashtable_itr_next(&itr);
	}
}
