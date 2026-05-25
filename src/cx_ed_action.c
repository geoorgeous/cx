#include <stdlib.h>
#include <string.h>

#include "cx_ed_action.h"

void cx_ed_action_history_execute(
	struct cx_ed_action_history* p_history,
	const struct cx_ed_action_def* p_def,
	void* p_ctx) {

	p_def->f_do(p_ctx);

	// increase stack size if required
	if (p_history->cursor == p_history->stack_cap) {
		p_history->stack_cap++;
		p_history->p_stack = realloc(p_history->p_stack, sizeof(*p_history->p_stack) * p_history->stack_cap);
	}

	// increase context buf size if required
	if (p_history->cursor_off + p_def->context_size < p_history->context_buf_size) {
		p_history->context_buf_size = p_history->cursor_off + p_def->context_size;
		p_history->p_context_buf = realloc(p_history->p_context_buf, p_history->context_buf_size);
	}

	// copy new action context in to buffer for later undo/redo usage
	uint8_t* p_context_dst = p_history->p_context_buf + p_history->cursor_off;
	memcpy(p_context_dst, p_ctx, p_def->context_size);

	p_history->p_stack[p_history->cursor] = (struct cx_ed_action) {
		.p_def = p_def,
		.p_context = p_context_dst
	};

	p_history->cursor++;
	p_history->cursor_off += p_def->context_size;

	p_history->count = p_history->cursor;
	p_history->count_off = p_history->cursor_off;
}

void cx_ed_action_history_undo(struct cx_ed_action_history* p_history) {
	if (p_history->cursor == 0) {
		return;
	}

	const struct cx_ed_action* p_action = p_history->p_stack + (p_history->cursor - 1);

	p_action->p_def->f_undo(p_action->p_context);

	p_history->cursor--;
	p_history->cursor -= p_action->p_def->context_size;
}

void cx_ed_action_history_redo(struct cx_ed_action_history* p_history) {
	if (p_history->cursor == p_history->count) {
		return;
	}

	const struct cx_ed_action* p_action = p_history->p_stack + p_history->cursor;

	p_action->p_def->f_do(p_action->p_context);

	p_history->cursor++;
	p_history->cursor_off += p_action->p_def->context_size;
}
