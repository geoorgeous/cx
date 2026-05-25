#ifndef CX_ED_ACTION_H
#define CX_ED_ACTION_H

#include <stddef.h>
#include <stdint.h>

typedef void (*cx_ed_action_do_fn)(void*);
typedef void (*cx_ed_action_undo_fn)(void*);

struct cx_ed_action_def {
	cx_ed_action_do_fn f_do;
	cx_ed_action_undo_fn f_undo;
	size_t context_size;
};

struct cx_ed_action {
	const struct cx_ed_action_def* p_def;
	void* p_context;
};

struct cx_ed_action_history {
	struct cx_ed_action* p_stack;
	size_t stack_cap;
	uint8_t* p_context_buf;
	size_t context_buf_size;
	size_t cursor;
	size_t count;
	size_t cursor_off;
	size_t count_off;
};

void cx_ed_action_history_execute(struct cx_ed_action_history* p_history,
	const struct cx_ed_action_def* p_def,
	void* p_ctx);
void cx_ed_action_history_undo(struct cx_ed_action_history* p_history);
void cx_ed_action_history_redo(struct cx_ed_action_history* p_history);

#endif
