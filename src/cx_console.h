#ifndef CX_CONSOLE_H
#define CX_CONSOLE_H

#include "cx_command_registry.h"
#include "cx_flog.h"
#include "cx_text_edit.h"


// console log view (log drawing, view state)
// console log view navigation


// todo:
//
// command history:
//    list management
//    navigation
//    drawing and tracking
//
// output log:
//    cache lines for easy indexing
//    navigation
//    drawing
//
// auto-completion
//    command name candidates
//    param candidates
//    candidate list drawing
//    candidate auto-complete
//    candidate cycling

#define CX_LOG_CAT_CONSOLE "console"

#define CX_CONSOLE_MAX_INPUT_LEN 1024

struct cx_console_input {
	char input_buf[CX_CONSOLE_MAX_INPUT_LEN];
	struct cx_text_edit text;
};

struct cx_console {
	int b_is_input_enabled;
	struct cx_command_registry command_registry;
	struct cx_console_input input;
	struct cx_flogger flogger;
};

struct cx_console* cx_console_get(void);

void cx_console_init(struct cx_console* p_console);
void cx_console_set_is_input_enabled(struct cx_console* p_console, int b_is_input_enabled);

#endif
