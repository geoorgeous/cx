#include "cx_cli.h"
#include "cx_commands.h"
#include "cx_logging.h"
#include "cx_text_input.h"
#include "input.h"
#include "keys.h"
#include <ctype.h>
#include <stdio.h>

#define CX_CLI_BUFFER_LEN 1024

static struct cx_text_input text_input;
static int b_input_enabled;

static void cx_cli_on_key(const void* p_event_data, void* p_user_ptr);
static void cx_cli_on_char(const void* p_event_data, void* p_user_ptr);
static void cx_cli_submit(void);

void cx_cli_enable(void) {
	if (b_input_enabled) {
		return;
	}
	
	b_input_enabled = 1;
	
	if (!text_input.p_buffer) {
		cx_text_input_init(&text_input, CX_CLI_BUFFER_LEN);
	}

	input_event_subscribe(INPUT_EVENT_key, cx_cli_on_key, 0);
	input_event_subscribe(INPUT_EVENT_char, cx_cli_on_char, 0);
}

void cx_cli_disable(void) {
	if (!b_input_enabled) {
		return;
	}
	b_input_enabled = 0;

	input_event_unsubscribe(INPUT_EVENT_key, cx_cli_on_key);
	input_event_unsubscribe(INPUT_EVENT_char, cx_cli_on_char);
}

int cx_cli_is_enabled(void) {
	return b_input_enabled;
}

void cx_cli_on_key(const void* p_event_data, void* p_user_ptr) {
	(void)p_user_ptr;

	const struct input_event_data_key* _p_event_data = p_event_data;

	if (!_p_event_data->b_is_down) {
		return;
	}

	switch(_p_event_data->key) {
		case KEY_enter: {
			cx_cli_submit();
			cx_text_input_clear(&text_input);
			break;
		}

		case KEY_backspace: {
			cx_text_input_delete(&text_input, -1);
			break;		
		}

		case KEY_delete: {
			cx_text_input_delete(&text_input, 1);
			break;
		}

		case KEY_left: {
			if (text_input.caret_pos > 0) {
				--text_input.caret_pos;
			}
			break;
		}

		case KEY_right: {
			if (text_input.caret_pos < (int)(text_input.input_len)) {
				++text_input.caret_pos;
			}
			break;
		}

		case KEY_escape: {
			cx_cli_disable();
			break;
		}

		default: break;
	}
}

void cx_cli_on_char(const void* p_event_data, void* p_user_ptr) {
	(void)p_user_ptr;

	const struct input_event_data_char* _p_event_data = p_event_data;

	if (iscntrl(_p_event_data->code)) {
		return;
	}

	const char str[2] = { _p_event_data->code, '\0' };

	cx_text_input_insert(&text_input, str);
}

void cx_cli_submit(void) {
	const char* s_command = 0;
	int         argc = 0;
	const char* argv[16];

	CX_LOG_FMT(INFO, DONTCARE, "Command string: \"%s\"\n", text_input.p_buffer);

	char  last = 0;
	char* p = text_input.p_buffer;

	while (*p) {
		if (!s_command) {
			if (!isspace(*p)) {
				s_command = p;
				last = *p;
			}
		} else {
			if (!isspace(last) && isspace(*p)) {
				last = *p;
				*p = '\0';
			}
			else if (isspace(last) && !isspace(*p)) {
				last = *p;
				argv[argc] = p;
				++argc;
			} else {
				last = *p;
			}
		}
		++p;
	}

	cx_commands_execute(s_command, argc, argv);
}
