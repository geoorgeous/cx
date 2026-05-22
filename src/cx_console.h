#ifndef CX_CONSOLE_H
#define CX_CONSOLE_H

#include "cx_alloc.h"
#include "cx_command_registry.h"
#include "cx_flog.h"
#include "cx_text_edit.h"
#include "cx_var_registry.h"

// todo:
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

#define CX_CONSOLE_MAX_INPUT_LEN 256
#define CX_CONSOLE_MAX_HISTORY_LEN 64

struct cx_console_input {
	char buf[CX_CONSOLE_MAX_INPUT_LEN];
	struct cx_text_edit text;
};

struct cx_console_history {
	struct cx_alloc_ring_entry entries[CX_CONSOLE_MAX_HISTORY_LEN];
	char history_buf[CX_CONSOLE_MAX_HISTORY_LEN][CX_CONSOLE_MAX_INPUT_LEN];
	char history_draf_buf[CX_CONSOLE_MAX_INPUT_LEN];
	struct cx_alloc_ring ring;
	int index;
};

#define CX_FLOG_ENTRIES_BUF_LEN 1024
#define CX_FLOG_STRING_BUF_SIZE CX_FLOG_ENTRIES_BUF_LEN * 1024
#define CX_FLOG_STYLES_BUF_LEN CX_FLOG_ENTRIES_BUF_LEN * 8
#define CX_FLOG_SPANS_BUF_LEN CX_FLOG_ENTRIES_BUF_LEN * 8

struct cx_console_flogger_storage {
	struct cx_alloc_ring_entry ring_strings_entries_buf_[CX_FLOG_ENTRIES_BUF_LEN];
	struct cx_alloc_ring_entry ring_styles_entries_buf_[CX_FLOG_ENTRIES_BUF_LEN];
	struct cx_alloc_ring_entry ring_spans_entries_buf_[CX_FLOG_ENTRIES_BUF_LEN];
	struct cx_alloc_ring_entry ring_entries_entries_buf_[CX_FLOG_ENTRIES_BUF_LEN];
	char ring_strings_buf_[CX_FLOG_STRING_BUF_SIZE];
	struct cx_flog_style ring_styles_buf_[CX_FLOG_STYLES_BUF_LEN];
	struct cx_flog_span ring_spans_buf_[CX_FLOG_SPANS_BUF_LEN];
	struct cx_flog_entry ring_entries_buf_[CX_FLOG_ENTRIES_BUF_LEN];
};

struct cx_console {
	int b_is_input_enabled;
	struct cx_command_registry command_registry;
	struct cx_var_registry var_registry;
	struct cx_console_input input;
	struct cx_console_history history;
	struct cx_flogger flogger;
	struct cx_console_flogger_storage flogger_storage;
};

struct cx_console* cx_console_get(void);

void cx_console_init(struct cx_console* p_console);
void cx_console_set_is_input_enabled(struct cx_console* p_console, int b_is_input_enabled);

#endif
