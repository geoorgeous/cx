#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cx_flog.h"
#include "cx_logging.h"
#include "cx_str.h"

static inline int cx_flog_style_cmp(const struct cx_flog_style* p_a, const struct cx_flog_style* p_b) {
	return cx_color_f32_cmp(&p_a->color, &p_b->color);
}

static inline void cx_flog_emit_span(struct cx_flog_builder* p_builder) {
	p_builder->p_spans[p_builder->num_spans] = (struct cx_flog_span) {
		.start = p_builder->active_style_begin,
		.len = p_builder->buf_len - p_builder->active_style_begin,
		.style_id = p_builder->style_stack[p_builder->stack_count - 1]
	};
	++p_builder->num_spans;
}

void cx_flog_end(struct cx_flogger* p_flogger, struct cx_flog_builder* p_builder) {
	if (p_builder->stack_count && p_builder->active_style_begin < p_builder->buf_len) {
		cx_flog_emit_span(p_builder);
	}

	const size_t string_size = p_builder->buf_len + 1;
	const size_t styles_size = p_builder->num_styles * sizeof(*p_builder->p_styles);
	const size_t spans_size = p_builder->num_spans * sizeof(*p_builder->p_spans);
	const size_t entry_size = sizeof(struct cx_flog_entry);

	while(!cx_alloc_ring_can_fit(&p_flogger->ring_strings_, string_size) ||
		!cx_alloc_ring_can_fit(&p_flogger->ring_styles_, styles_size) ||
		!cx_alloc_ring_can_fit(&p_flogger->ring_spans_, spans_size) ||
		!cx_alloc_ring_can_fit(&p_flogger->ring_entries_, entry_size)) {
		
		cx_alloc_ring_pop(&p_flogger->ring_strings_);
		cx_alloc_ring_pop(&p_flogger->ring_styles_);
		cx_alloc_ring_pop(&p_flogger->ring_spans_);
		cx_alloc_ring_pop(&p_flogger->ring_entries_);
	}

	const struct cx_flog_entry new_entry = {
		.s = cx_alloc_ring_push(
			&p_flogger->ring_strings_,
			p_builder->p_buf,
			string_size,
			CX_ALLOC_RING_PUSH_POLICY_no),
		.p_styles = cx_alloc_ring_push(
			&p_flogger->ring_styles_, 
			p_builder->p_styles, 
			styles_size, 
			CX_ALLOC_RING_PUSH_POLICY_no),
		.num_styles = p_builder->num_styles,
		.p_spans = cx_alloc_ring_push(
			&p_flogger->ring_spans_,
			p_builder->p_spans,
			spans_size,
			CX_ALLOC_RING_PUSH_POLICY_no),
		.num_spans = p_builder->num_spans
	};

	CX_LOG(INFO, FLOG, "New entry:\n");
	CX_LOG_FMT(INFO, DONTCARE, "  content:\n%s\n", new_entry.s);
	CX_LOG_FMT(INFO, DONTCARE, "  styles: %d\n", new_entry.num_styles);

	for (size_t i = 0; i < new_entry.num_styles; ++i) {
		const struct cx_flog_style* p_style = new_entry.p_styles + i;
		CX_LOG_FMT(INFO, DONTCARE, "    %d: color=[%g, %g, %g, %g]\n",
			p_style->color.r, p_style->color.g, p_style->color.b, p_style->color.a);
	}

	CX_LOG_FMT(INFO, DONTCARE, "  spans: %d\n", new_entry.num_styles);

	for (size_t i = 0; i < new_entry.num_spans; ++i) {
		const struct cx_flog_span* p_span = new_entry.p_spans + i;
		CX_LOG_FMT(INFO, DONTCARE, "    %d: [%d...%d], style_id=%d\n", p_span->start, p_span->len, p_span->style_id);
	}
	
	cx_alloc_ring_push(&p_flogger->ring_entries_, &new_entry, entry_size, CX_ALLOC_RING_PUSH_POLICY_no);

	*p_builder = (struct cx_flog_builder) {
		.p_buf = p_builder->p_buf,
		.p_styles = p_builder->p_styles,
		.p_spans = p_builder->p_spans
	};
}

void cx_flog_append(struct cx_flog_builder* p_builder, const char* s) {
	char* p_start = p_builder->p_buf + p_builder->buf_len;
	char* p_end = cx_stpcpy(p_start, s);
	p_builder->buf_len += p_end - p_start;
}

void cx_flog_append_fmt(struct cx_flog_builder* p_builder, const char* s, ...) {
	char temp[1024];

	va_list vargs;
	va_start(vargs, s);
	const size_t len = vsprintf(temp, s, vargs);
	va_end(vargs);

	strcpy(p_builder->p_buf + p_builder->buf_len, temp);
	p_builder->buf_len += len;
}

void cx_flog_push_style(struct cx_flog_builder* p_builder, struct cx_flog_style style) {
	if (p_builder->stack_count && p_builder->active_style_begin < p_builder->buf_len) {
		cx_flog_emit_span(p_builder);
	}
	
	uint8_t style_id = p_builder->num_styles;
	for (size_t i = 0; i < p_builder->num_styles; ++i) {
		if (cx_flog_style_cmp(p_builder->p_styles + i, &style)) {
			style_id = i;
			break;
		}
	}

	if (style_id == p_builder->num_styles) {
		p_builder->p_styles[style_id] = style;
		++p_builder->num_styles;
	}

	p_builder->style_stack[p_builder->stack_count++] = style_id;
	p_builder->active_style_begin = p_builder->buf_len;
}

void cx_flog_pop_style(struct cx_flog_builder* p_builder) {
	if (p_builder->stack_count == 0 || p_builder->active_style_begin == p_builder->buf_len) {
		return;
	}

	cx_flog_emit_span(p_builder);

	--p_builder->stack_count;
	p_builder->active_style_begin = p_builder->buf_len;
}
