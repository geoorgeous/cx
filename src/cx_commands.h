#ifndef _H__CX_COMMANDS

#define CX_LOG_CAT_COMMANDS "commands"

typedef void(*command_proc)(int, const char*[]);

void cx_commands_register(const char* s_command, command_proc f_proc);
void cx_commands_unregister(const char* s_command);
void cx_commands_execute(const char* s_command, int argc, const char* argv[]);

#endif
