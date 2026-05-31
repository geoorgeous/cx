#include <stdlib.h>
#include <string.h>

#include "cx_ed_action.h"
#include "cx_logging.h"

void cx_ed_action_history_execute(
	struct cx_ed_action_history* p_history,
	const struct cx_ed_action_def* p_def,
	void* p_ctx) {

	p_def->f_do(p_ctx);

	// increase stack size if required
	if (p_history->cursor == p_history->stack_cap) {
		p_history->stack_cap = p_history->stack_cap ? p_history->stack_cap * 2 : 8;
		p_history->p_stack = realloc(p_history->p_stack, sizeof(*p_history->p_stack) * p_history->stack_cap);
	}

	size_t context_off = 0;

	if (p_history->cursor > 0) {
		const struct cx_ed_action* p_prev_action = &p_history->p_stack[p_history->cursor - 1];
		context_off = CX_ALIGN_UP(
			p_prev_action->context_off + p_prev_action->p_def->context_size,
			p_def->context_alignment);
	}

	// increase context buf size if required
	if (context_off + p_def->context_size > p_history->context_buf_size) {
		p_history->context_buf_size += context_off + p_def->context_size;
		p_history->p_context_buf = realloc(p_history->p_context_buf, p_history->context_buf_size);
	}

	// copy new action context in to buffer for later undo/redo usage
	uint8_t* p_context_dst = p_history->p_context_buf + context_off;
	memcpy(p_context_dst, p_ctx, p_def->context_size);

	p_history->p_stack[p_history->cursor] = (struct cx_ed_action) {
		.p_def = p_def,
		.context_off = context_off
	};

	p_history->cursor++;
	p_history->count = p_history->cursor;
}

void cx_ed_action_history_undo(struct cx_ed_action_history* p_history) {
	if (p_history->cursor == 0) {
		CX_LOG(INFO, ACTION, "Already at oldest change\n");
		return;
	}

	const struct cx_ed_action* p_action = p_history->p_stack + (p_history->cursor - 1);
	void* p_context = p_history->p_context_buf + p_action->context_off;

	p_action->p_def->f_undo(p_context);

	p_history->cursor--;
}

void cx_ed_action_history_redo(struct cx_ed_action_history* p_history) {
	if (p_history->cursor == p_history->count) {
		CX_LOG(INFO, ACTION, "Already at newest change\n");
		return;
	}

	const struct cx_ed_action* p_action = &p_history->p_stack[p_history->cursor];
	void* p_context = p_history->p_context_buf + p_action->context_off;

	p_action->p_def->f_do(p_context);

	p_history->cursor++;
}
