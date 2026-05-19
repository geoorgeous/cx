#ifndef CX_FLOG_H
#define CX_FLOG_H

#include <stddef.h>
#include <stdint.h>

#include "cx_color.h"

#define CX_FLOG_NOSTYLE 0

struct cx_flog_style {
	struct cx_color_f32 color;
};

struct cx_flog_span {
	size_t start;
	size_t len;
	struct cx_flog_style style;
};

struct cx_flog_entry {
	size_t offset;
	size_t len;
	size_t spans_first;
	size_t num_spans;
};

struct cx_flogger {
	char* p_text_buffer;
	size_t text_buffer_cap;
	size_t text_buffer_len;
	size_t text_buffer_head;
	size_t text_buffer_tail;
	struct cx_flog_entry* p_entry_buffer;
	size_t entry_buffer_cap;
	size_t entry_buffer_len;
	size_t entry_buffer_head;
	size_t entry_buffer_tail;
	struct cx_flog_span* p_span_buffer;
	size_t span_buffer_cap;
	size_t span_buffer_len;
	size_t span_buffer_head;
	size_t span_buffer_tail;
};

void cx_flog_init(struct cx_flogger* p_flogger);
void cx_flog_free(struct cx_flogger* p_flogger);
void cx_flog(struct cx_flogger* p_flogger, const struct cx_flog_style* p_style, const char* s_text);
void cx_flogf(struct cx_flogger* p_flogger, const struct cx_flog_style* p_style, const char* s_text, ...);
void cx_flog_end(struct cx_flogger* p_flogger);

#endif
