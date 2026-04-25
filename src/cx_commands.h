#ifndef _H__CX_COMMANDS

#define CX_LOG_CAT_COMMANDS "commands"

struct cx_command_info {
	const char* s_id;
	char*       s_description;
	void*       p_data_ptr;
};

struct cx_command_context {
	const struct cx_command_info* p_command;
	int                           argc;
	const char**                  s_args;
};

typedef void(*cx_command_proc)(const struct cx_command_context* p_context);

void cx_commands_register(const struct cx_command_info* p_command_info, cx_command_proc f_proc);
void cx_commands_unregister(const char* s_command_id);
void cx_commands_execute(const char* s_command_id, int argc, const char* argv[]);

#endif
