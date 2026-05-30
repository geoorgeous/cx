#ifndef CX_FLOG_H
#define CX_FLOG_H

#include <stddef.h>
#include <stdint.h>

#include "cx_alloc.h"
#include "cx_color.h"

#define CX_LOG_CAT_FLOG "flog"

#define CX_FLOG_NOSTYLE 0

struct cx_flog_style {
	struct cx_color color;
};

struct cx_flog_span {
	uint32_t start;
	uint32_t len;
	uint8_t style_id;
};

struct cx_flog_entry {
	const char* s;
	const struct cx_flog_style* p_styles;
	uint8_t num_styles;
	const struct cx_flog_span* p_spans;
	uint8_t num_spans;
};

struct cx_flog_builder {
	char* p_buf;
	uint32_t buf_len;
	struct cx_flog_style* p_styles; // list of style
	uint8_t num_styles;
	struct cx_flog_span* p_spans; // list of spans
	uint8_t num_spans;
	uint8_t style_stack[32]; // stack of style ids
	uint8_t stack_count;
	uint32_t active_style_begin;
};

struct cx_flogger {
	struct cx_alloc_ring ring_strings_;
	struct cx_alloc_ring ring_styles_;
	struct cx_alloc_ring ring_spans_;
	struct cx_alloc_ring ring_entries_;
};

void cx_flog_end(struct cx_flogger* p_flogger, struct cx_flog_builder* p_builder);

void cx_flog_append(struct cx_flog_builder* p_builder, const char* s);
void cx_flog_append_fmt(struct cx_flog_builder* p_builder, const char* s, ...);
void cx_flog_push_style(struct cx_flog_builder* p_builder, struct cx_flog_style style);
void cx_flog_pop_style(struct cx_flog_builder* p_builder);


#endif
