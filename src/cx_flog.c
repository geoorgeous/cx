#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cx_flog.h"

#define CX_FLOG_TEXT_BUFFER_INITIAL_LEN 1024
#define CX_FLOG_TEXT_BUFFER_MAX_LEN (1024 * 1024)
#define CX_FLOG_ENTRY_BUFFER_INITIAL_LEN (CX_FLOG_TEXT_BUFFER_INITIAL_LEN / 100)
#define CX_FLOG_ENTRY_BUFFER_MAX_LEN (CX_FLOG_TEXT_BUFFER_MAX_LEN / 100)
#define CX_FLOG_SPAN_BUFFER_INITIAL_LEN CX_FLOG_ENTRY_BUFFER_INITIAL_LEN
#define CX_FLOG_SPAN_BUFFER_MAX_LEN CX_FLOG_ENTRY_BUFFER_MAX_LEN

void cx_flog_internal(
	struct cx_flogger* p_flogger,
	const struct cx_flog_style* p_style,
	const char* s_text,
	size_t text_len);

void cx_flog_init(struct cx_flogger* p_flogger) {
	*p_flogger = (struct cx_flogger){
		.entry_buffer_cap = CX_FLOG_ENTRY_BUFFER_INITIAL_LEN,
		.text_buffer_cap = CX_FLOG_TEXT_BUFFER_INITIAL_LEN,
		.span_buffer_cap = CX_FLOG_SPAN_BUFFER_INITIAL_LEN
	};

	const size_t entry_buffer_size = p_flogger->entry_buffer_cap * sizeof(*p_flogger->p_entry_buffer);
	const size_t text_buffer_size = p_flogger->text_buffer_cap * sizeof(*p_flogger->p_text_buffer);
	const size_t span_buffer_size = p_flogger->span_buffer_cap * sizeof(*p_flogger->p_span_buffer);

	p_flogger->p_entry_buffer = malloc(entry_buffer_size + text_buffer_size + span_buffer_size);
	p_flogger->p_text_buffer = (void*)((char*)p_flogger->p_entry_buffer + entry_buffer_size);
	p_flogger->p_span_buffer = (void*)((char*)p_flogger->p_text_buffer + text_buffer_size);
}

void cx_flog_free(struct cx_flogger* p_flogger) {
	free(p_flogger->p_text_buffer);
	free(p_flogger->p_entry_buffer);
	free(p_flogger->p_span_buffer);
	*p_flogger = (struct cx_flogger){0};
}

void cx_flog(struct cx_flogger* p_flogger, const struct cx_flog_style* p_style, const char* s_text) {
	const size_t text_len = strlen(s_text);
	cx_flog_internal(p_flogger, p_style, s_text, text_len);
}

void cx_flogf(struct cx_flogger* p_flogger, const struct cx_flog_style* p_style, const char* s_fmt, ...) {
	char buf[1024];

    va_list args;
    va_start(args, s_fmt);

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
	const size_t text_len = vsprintf(buf, s_fmt, args);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNU__)
#pragma GCC diagnostic pop
#endif

    va_end(args);

	cx_flog_internal(p_flogger, p_style, buf, text_len);
}

void cx_flog_end(struct cx_flogger* p_flogger) {
	if (p_flogger->p_entry_buffer[p_flogger->entry_buffer_head].len == 0) {
		// Silently ignore empty entries
		return;
	}

	++p_flogger->entry_buffer_len;

	struct cx_flog_entry* p_entry; // todo: allocate

	*p_entry = (struct cx_flog_entry) {
		.offset = p_flogger->text_buffer_head,
		.spans_first = p_flogger->span_buffer_head
	};
}

void cx_flog_internal(
	struct cx_flogger* p_flogger,
	const struct cx_flog_style* p_style,
	const char* s_text,
	size_t text_len) {

	struct cx_flog_entry* p_entry = &p_flogger->p_entry_buffer[p_flogger->entry_buffer_head];

	if (p_style != CX_FLOG_NOSTYLE) {
		struct cx_flog_span* p_span; // todo: allocate span
		*p_span = (struct cx_flog_span) {
			.start = p_entry->len,
			.len = text_len,
			.style = *p_style
		};
		++p_entry->num_spans;
	}

	// allocate string
	// copy
	
	p_entry->len += text_len;

}
