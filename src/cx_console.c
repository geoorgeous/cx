#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cx_command.h"
#include "cx_console.h"
#include "cx_logging.h"
#include "cx_macro.h"
#include "cx_var.h"
#include "input.h"
#include "keys.h"

#define CX_CONSOLE_LOG_LINE_BREAK "------------------------------------------------------------"

static struct cx_console console;

static void cx_console_history_set(struct cx_console* p_console, int index);
static void cx_console_on_key(const void*, void*);
static void cx_console_on_char(const void*, void*);
static int cx_console_command_clear(const struct cx_command_args* p_args, const struct cx_command_context* p_context);
static int cx_console_command_help(const struct cx_command_args* p_args, const struct cx_command_context* p_context);
static int cx_console_command_alias(const struct cx_command_args* p_args, const struct cx_command_context* p_context);
static int cx_console_command_unalias(
	const struct cx_command_args* p_args,
	const struct cx_command_context* p_context);
static int cx_console_command_var(const struct cx_command_args* p_args, const struct cx_command_context* p_context);
static int cx_console_command_history(
	const struct cx_command_args* p_args,
	const struct cx_command_context* p_context);
static int cx_console_command_test(const struct cx_command_args* p_args, const struct cx_command_context* p_context);

struct cx_console* cx_console_get(void) {
	return &console;
}

void cx_console_init(struct cx_console* p_console) {
	p_console->command_registry = (struct cx_command_registry){0};
	
	cx_text_edit_set_buf(&p_console->input.text, p_console->input.buf, CX_CONSOLE_MAX_INPUT_LEN);

	CX_ALLOC_RING_INIT_ARRAYS(&p_console->history.ring, p_console->history.history_buf, p_console->history.entries);
	
	CX_ALLOC_RING_INIT_ARRAYS(
		&p_console->flogger.ring_strings_,
		p_console->flogger_storage.ring_strings_buf_,
		p_console->flogger_storage.ring_strings_entries_buf_);

	CX_ALLOC_RING_INIT_ARRAYS(
		&p_console->flogger.ring_styles_,
		p_console->flogger_storage.ring_styles_buf_,
		p_console->flogger_storage.ring_styles_entries_buf_);

	CX_ALLOC_RING_INIT_ARRAYS(
		&p_console->flogger.ring_spans_,
		p_console->flogger_storage.ring_spans_buf_,
		p_console->flogger_storage.ring_spans_entries_buf_);

	CX_ALLOC_RING_INIT_ARRAYS(
		&p_console->flogger.ring_entries_,
		p_console->flogger_storage.ring_entries_buf_,
		p_console->flogger_storage.ring_entries_entries_buf_);

	CX_NEW_COMMAND(clear,
		"Clear the console output",
		cx_console_command_clear, 0);

	CX_NEW_COMMAND(help,
		"List all commands, or the details of a single command",
		cx_console_command_help, &console.command_registry,
		CX_COMMAND_PARAM(STRING("name", "Command to list detail of"), OPTIONAL));
	CX_NEW_COMMAND_ALIAS("h", "help");

	CX_NEW_COMMAND(alias,
		"List, create, or query aliases",
		cx_console_command_alias, &console.command_registry,
		CX_COMMAND_PARAM(STRING("name", "Alias name"), OPTIONAL),
		CX_COMMAND_PARAM(STRING("expansion", "Expansion to execute"), OPTIONAL));

	CX_NEW_COMMAND(unalias,
		"Delete an alias",
		cx_console_command_unalias, &console.command_registry,
		CX_COMMAND_PARAM(STRING("name", "Name of the alias to delete"), REQUIRED));

	CX_NEW_COMMAND(var,
		"List, query, or assign vars",
		cx_console_command_var, &console.var_registry,
		CX_COMMAND_PARAM(STRING("name", "Name of the variable to get/set"), OPTIONAL),
		CX_COMMAND_PARAM(STRING("value", "Value to assign to the variable"), OPTIONAL));

	CX_NEW_COMMAND(history,
		"List command history",
		cx_console_command_history, &console.history);

	static const struct cx_var_enum_map_entry entries[] = {
		{ "eone",   0 },
		{ "etwo",   1 },
		{ "ethree", 2 },
		{ "efour",  3 },
		{ "efive",  4 }
	};

	CX_NEW_COMMAND(test,
		"Testing, tesing, one, two, three",
		cx_console_command_test, 0,
		CX_COMMAND_PARAM(STRING("one", "First parameter"), REQUIRED),
		CX_COMMAND_PARAM(INT("two", "Second parameter"), REQUIRED),
		CX_COMMAND_PARAM(INT_RANGE("two", "Second parameter", 0, 100), REQUIRED),
		CX_COMMAND_PARAM(FLOAT("three", "Third parameter"), REQUIRED),
		CX_COMMAND_PARAM(FLOAT_RANGE("three", "Third parameter", -0.5f, 0.5f), REQUIRED),
		CX_COMMAND_PARAM(BOOL("four", "Four parameter"), REQUIRED),
		CX_COMMAND_PARAM(ENUM("seven", "", entries, 5), REQUIRED));
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
			cx_console_history_set(p_console, p_console->history.index + 1);
			break;
		}
		case KEY_down: {
			cx_console_history_set(p_console, p_console->history.index - 1);
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
			cx_alloc_ring_push(
				&p_console->history.ring,
				p_console->input.text.p_buf,
				p_console->input.text.len + 1,
				CX_ALLOC_RING_PUSH_POLICY_auto);
			cx_command_registry_execute(
				&p_console->command_registry,
				p_console->input.text.p_buf,
				&p_console->flogger);
			cx_console_history_set(p_console, -1);
			cx_text_edit_clear(&p_console->input.text);
			break;
		}
		case KEY_tab: {
			break;
		}
		case KEY_escape: {
			cx_console_set_is_input_enabled(p_console, 0);
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

static int cx_console_command_clear(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_args;
	cx_alloc_ring_clear(&p_context->p_flogger->ring_entries_);
	cx_alloc_ring_clear(&p_context->p_flogger->ring_strings_);
	cx_alloc_ring_clear(&p_context->p_flogger->ring_styles_);
	cx_alloc_ring_clear(&p_context->p_flogger->ring_spans_);
	return 0;
}

int cx_console_command_help(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	const struct cx_command_registry* p_registry = p_context->p_command->p_user_ptr;
	
	char flog_str_buf[1024];
	struct cx_flog_style flog_style_buf[2];
	struct cx_flog_span flog_span_buf[8];
	struct cx_flog_builder flog = {
		.p_buf = flog_str_buf,
		.p_styles = flog_style_buf,
		.p_spans = flog_span_buf
	};

	if (p_args->count > 0) {
		const struct cx_command* p_command;
		if (!cx_command_registry_find(p_registry, p_args->list[0].as_str.p, &p_command)) {
			cx_flog_append_fmt(&flog, "command %s not found", p_args->list[0].as_str);
		} else {
			cx_flog_append_fmt(&flog,
				"%s\n"
				CX_CONSOLE_LOG_LINE_BREAK"\n"
				"%s\n\n"
				"Usage:\n"
				"  %s ", p_command->s_name, p_command->s_desc, p_command->s_name);

			const char lbr[] = { '[', '<' };
			const char rbr[] = { ']', '>' };

			size_t max_len = 0;
			for (size_t i = 0; i < p_command->num_params; ++i) {
				const struct cx_command_param* p_param = p_command->p_params + i;
				
				// calculate longest name for later padding
				size_t len = strlen(p_param->desc.s_name);
				if (len > max_len) {
					max_len = len;
				}

				cx_flog_append_fmt(&flog, "%c%s%c ",
					lbr[p_param->b_required], p_param->desc.s_name, rbr[p_param->b_required]);
			}

			if (p_command->num_params > 0) {
				cx_flog_append(&flog, "\n\nParams: (> required, - optional)");

				for (size_t i = 0; i < p_command->num_params; ++i) {
					const struct cx_command_param* p_param = p_command->p_params + i;
					cx_flog_append_fmt(&flog, "\n  %c %-*s  %-6s  %s",
						p_param->b_required ? '>' : '-',
						max_len, p_param->desc.s_name,
						cx_var_type_str(p_param->desc.type),
						p_param->desc.s_desc);

					// todo: print enum values, numeric constraints
					if (p_param->desc.type == CX_VAR_TYPE_int) {
					} else if (p_param->desc.type == CX_VAR_TYPE_float) {
					} else if (p_param->desc.type == CX_VAR_TYPE_enum) {
					}
				}
			}
		}
	} else {
		cx_flog_append_fmt(&flog, "Commands (%d)\n", p_registry->num_commands_);
		cx_flog_append(&flog, CX_CONSOLE_LOG_LINE_BREAK"\n");

		for (size_t i = 0; i < p_registry->num_commands_; ++i) {
			const struct cx_command* p_command = p_registry->pp_commands_[i];
			cx_flog_append_fmt(&flog, " %s\n", p_command->s_name);
		}
		cx_flog_append(&flog, CX_CONSOLE_LOG_LINE_BREAK"\n"
			" type: help <command> for details");
	}

	cx_flog_end(p_context->p_flogger, &flog);

	return 0;
}

int cx_console_command_alias(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	struct cx_command_registry* p_registry = p_context->p_command->p_user_ptr;

	char flog_str_buf[1024];
	struct cx_flog_style flog_style_buf[8];
	struct cx_flog_span flog_span_buf[8];
	struct cx_flog_builder flog = {
		.p_buf = flog_str_buf,
		.p_styles = flog_style_buf,
		.p_spans = flog_span_buf
	};

	if (p_args->count == 0) {
		cx_flog_append_fmt(&flog, "Aliases (%d)\n", p_registry->num_aliases_);

		if (p_registry->num_aliases_ == 0) {
			cx_flog_end(p_context->p_flogger, &flog);
			return 0;
		}

		cx_flog_append(&flog, CX_CONSOLE_LOG_LINE_BREAK"\n");
		for (size_t i = 0; i < p_registry->num_aliases_; ++i) {
			const struct cx_command_alias* p_alias = &p_registry->p_aliases_[i];
			cx_flog_append_fmt(&flog, " %s -> \"%s\"\n", p_alias->s_name, p_alias->s_expansion);
		}
		cx_flog_append(&flog, CX_CONSOLE_LOG_LINE_BREAK);
		cx_flog_end(p_context->p_flogger, &flog);
		return 0;
	}

	char* p_alias_name = strndup(p_args->list[0].as_str.p, p_args->list[0].as_str.len);

	const struct cx_command_alias* p_alias;
	const int b_found = cx_command_registry_find_alias(p_registry, p_alias_name, &p_alias);

	if (p_args->count == 1) {
		if (!b_found) {
			cx_flog_append_fmt(&flog, "alias %s not found", p_alias_name);
			free(p_alias_name);
			cx_flog_end(p_context->p_flogger, &flog);
			return 1;
		}
		cx_flog_append_fmt(&flog, "alias: %s -> \"%s\"", p_alias->s_name, p_alias->s_expansion);
		free(p_alias_name);
		cx_flog_end(p_context->p_flogger, &flog);
		return 0;
	}

	if (b_found) {
		// already exists
		free(p_alias_name);
		return 2;	
	}

	char* p_alias_expansion = strndup(p_args->list[1].as_str.p, p_args->list[1].as_str.len);

	cx_command_registry_add_alias(p_registry, p_alias_name, p_alias_expansion);

	free(p_alias_name);
	free(p_alias_expansion);

	return 0;
}

int cx_console_command_unalias(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	struct cx_command_registry* p_registry = p_context->p_command->p_user_ptr;
	char* p_alias_name = strndup(p_args->list[0].as_str.p, p_args->list[0].as_str.len);
	cx_command_registry_remove_alias(p_registry, p_alias_name);
	free(p_alias_name);
	return 0;
}

int cx_console_command_var(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	struct cx_var_registry* p_registry = p_context->p_command->p_user_ptr;

	if (p_args->count == 0) {
		// list vars
		return 0;
	}
	
	char* p_var_name = strndup(p_args->list[0].as_str.p, p_args->list[0].as_str.len);

	const struct cx_var* p_var;
	if (!cx_var_registry_find(p_registry, p_var_name, &p_var)) {
		// var not found
		free(p_var_name);
		return -1;
	}

	if (p_args->count == 1) {
		free(p_var_name);
		return 0;
	}

	cx_var_try_set(p_var, p_args->list[1].as_str.p, p_args->list[1].as_str.len);

	free(p_var_name);

	return 0;
}

int cx_console_command_history(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_args;

	char flog_str_buf[1024];
	struct cx_flog_builder flog = {
		.p_buf = flog_str_buf,
	};

	const struct cx_console_history* p_history = p_context->p_command->p_user_ptr;
	cx_flog_append_fmt(&flog, "Command history (%d)", p_history->ring.entries_count_ - 1);
	if (p_history->ring.entries_count_ > 1) {
		cx_flog_append(&flog, "\n"CX_CONSOLE_LOG_LINE_BREAK);
		for (size_t i = 1; i < p_history->ring.entries_count_; ++i) {
			size_t size;
			const char* s_history_entry = cx_alloc_ring_get(&p_history->ring, i, &size);
			cx_flog_append_fmt(&flog, "\n(%d) %s", i, s_history_entry);
		}
		cx_flog_append(&flog, "\n"CX_CONSOLE_LOG_LINE_BREAK);
	}

	cx_flog_end(p_context->p_flogger, &flog);

	return 0;
}

int cx_console_command_test(const struct cx_command_args* p_args, const struct cx_command_context* p_context) {
	(void)p_context;
	CX_LOG_FMT(INFO, COMMAND, "test args: \"%.*s\", %d, %d, %g, %g, %d, %s(%d)\n",
		p_args->list[0].as_str.len, p_args->list[0].as_str.p,
		p_args->list[1].as_int,
		p_args->list[2].as_int,
		p_args->list[3].as_float,
		p_args->list[4].as_float,
		p_args->list[5].b_as_bool,
		p_args->list[6].p_as_enum->s_name, p_args->list[6].p_as_enum->value);
	return 0;
}

void cx_console_history_set(struct cx_console* p_console, int index) {
	if (index > (int)p_console->history.ring.entries_count_ - 1) {
		index = p_console->history.ring.entries_count_ - 1;
	}
	if (index < 0) {
		index = -1;
	}
	if (index == p_console->history.index) {
		return;
	}
	p_console->history.index = index;

	if (p_console->history.index == -1) {
		cx_text_edit_set_buf(&p_console->input.text, p_console->input.buf, CX_CONSOLE_MAX_INPUT_LEN);
		return;
	}

	size_t size;
	const char* s_history = cx_alloc_ring_get(&p_console->history.ring, index, &size);
	strcpy(p_console->history.history_draf_buf, s_history);

	cx_text_edit_set_buf(&p_console->input.text, p_console->history.history_draf_buf, CX_CONSOLE_MAX_INPUT_LEN);
}
