#include <stdlib.h>
#include <string.h>

#include "cx_text_input.h"

void cx_text_input_init(struct cx_text_input* p_text_input, unsigned int max_input_len) {
	const unsigned int buffer_size = max_input_len + 1;
	*p_text_input = (struct cx_text_input) {
		.p_buffer = malloc(buffer_size),
		.buffer_size = buffer_size
	};
}

void cx_text_input_free(struct cx_text_input* p_text_input) {
	free(p_text_input->p_buffer);
	*p_text_input = (struct cx_text_input){0};
}

void cx_text_input_clear(struct cx_text_input* p_text_input) {
	p_text_input->input_len = 0;
	p_text_input->caret_pos = 0;
	*p_text_input->p_buffer = '\0';
}

void cx_text_input_insert(struct cx_text_input* p_text_input, const char* s) {
	size_t len = strlen(s);
	if (p_text_input->input_len + len >= p_text_input->buffer_size) {
		len = p_text_input->buffer_size - p_text_input->input_len - 1;
	}

	const char*  p_src = s;
	char*        p_dst = p_text_input->p_buffer + p_text_input->caret_pos;

	memcpy(p_dst + len, p_dst, p_text_input->input_len - p_text_input->caret_pos);
	memcpy(p_dst, p_src, len);
	
	p_text_input->input_len += len;
	p_text_input->caret_pos += len;
	p_text_input->p_buffer[p_text_input->input_len] = '\0';
}

void cx_text_input_delete(struct cx_text_input* p_text_input, int n) {
	int first, last;

	// geo^rge

	if (n > 0) {
		first = p_text_input->caret_pos;
		last = first + n;

		if (last > (int)p_text_input->input_len) {
			last = p_text_input->input_len;
		}
	} else if (n < 0) {
		last = p_text_input->caret_pos;
		first = last + n;

		if (first < 0) {
			first = 0;
		}
	} else {
		return;
	}

	if ((last - first) <= 0) {
		return;
	}

	memcpy(p_text_input->p_buffer + first, p_text_input->p_buffer + last, p_text_input->input_len - last + 1);

	p_text_input->input_len -= last - first;
	p_text_input->caret_pos = first;
	p_text_input->p_buffer[p_text_input->input_len] = '\0';
}
