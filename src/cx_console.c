#include <ctype.h>

#include "cx_command.h"
#include "cx_console.h"
#include "cx_logging.h"
#include "input.h"
#include "keys.h"

	// command_name <boolean_param:b> <string_param> <enum_pram:val0|val1|val2}> [int_param:i] [float_param:f]
	// 
	// Description blah blah this command does something really useful and cool and powerful
	// 
	//  > boolean_param (bool): Description
	//  > string_param (string): Description
	//  > enum_param (val0|val1|val2): Description
	//  - int_param (int): Description
	//  - float_param (float): Description

static struct cx_console console;

static void cx_console_on_key(const void*, void*);
static void cx_console_on_char(const void*, void*);
static int cx_console_command_help(size_t argc, const union cx_command_arg* p_args, struct cx_flogger* p_flogger);

struct cx_console* cx_console_get(void) {
	return &console;
}

void cx_console_init(struct cx_console* p_console) {
	p_console->command_registry = (struct cx_command_registry){0};

	p_console->input.text = (struct cx_text_edit) {
		.p_buf = p_console->input.input_buf,
		.buf_size = CX_CONSOLE_MAX_INPUT_LEN
	};

	cx_flog_init(&p_console->flogger);

	const static struct cx_command_param params[] = {
		CX_COMMAND_PARAM(STRING("command", "Name of command"), 0)
	};
	const static struct cx_command help = {
		.s_name = "help",
		.s_desc = "Help command",
		.p_params = params,
		.num_params = 1,
		.f = cx_console_command_help,
	};
	cx_command_registry_add(&p_console->command_registry, &help);
}

void cx_console_set_is_input_enabled(struct cx_console* p_console, int b_is_input_enabled) {
	if (!!p_console->b_is_input_enabled == !!b_is_input_enabled) {
		return;
	}

	if (b_is_input_enabled) {
		input_event_subscribe(INPUT_EVENT_key, cx_console_on_key, p_console);
		input_event_subscribe(INPUT_EVENT_char, cx_console_on_char, p_console);
	} else {
		input_event_unsubscribe(INPUT_EVENT_key, cx_console_on_key);
		input_event_unsubscribe(INPUT_EVENT_char, cx_console_on_char);
	}

	p_console->b_is_input_enabled = !!b_is_input_enabled;
}

void cx_console_on_key(const void* p_event, void* p_user_ptr) {
	const struct input_event_data_key* p_e = p_event;
	struct cx_console* p_console = p_user_ptr;

	if (!p_e->b_is_down) {
		return;
	}

	switch (p_e->key) {
		case KEY_left: {
			if (p_e->mods & INPUT_MOD_ctrl) {
				cx_text_edit_cursor_prev_word(&p_console->input.text);
			} else {
				cx_text_edit_cursor_offset(&p_console->input.text, -1);
			}
			break;
		}
		case KEY_right: {
			if (p_e->mods & INPUT_MOD_ctrl) {
				cx_text_edit_cursor_next_word(&p_console->input.text);
			} else {
				cx_text_edit_cursor_offset(&p_console->input.text, 1);
			}
			break;
		}
		case KEY_up: {
			// history back
			break;
		}
		case KEY_down: {
			// history forward
			break;
		}
		case KEY_backspace: {
			cx_text_edit_delete(&p_console->input.text, -1);
			break;
		}
		case KEY_delete: {
			cx_text_edit_delete(&p_console->input.text, 1);
			break;
		}
		case KEY_home: {
			cx_text_edit_cursor_set(&p_console->input.text, 0);
			break;
		}
		case KEY_end: {
			cx_text_edit_cursor_set(&p_console->input.text, p_console->input.text.len);
			break;
		}
		case KEY_enter: {
			cx_command_registry_execute(&p_console->command_registry, p_console->input.input_buf, &p_console->flogger);
			cx_text_edit_clear(&p_console->input.text);
			break;
		}
		default: break;
	}
}

void cx_console_on_char(const void* p_event, void* p_user_ptr) {
	const struct input_event_data_char* p_e = p_event;
	struct cx_console* p_console = p_user_ptr;
	char c = p_e->code;

	if (iscntrl(c)) {
		return;
	}

	cx_text_edit_insert(&p_console->input.text, &c, 1);
}

int cx_console_command_help(size_t argc, const union cx_command_arg* p_args, struct cx_flogger* p_flogger) {
	CX_LOG(INFO, CONSOLE, "Help command\n");
	return 0;
}
