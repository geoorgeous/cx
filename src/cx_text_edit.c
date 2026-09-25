#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "cx_math.h"
#include "cx_str.h"
#include "cx_text_edit.h"

void cx_text_edit_set_buf(struct cx_text_edit* p_text_edit, char* p_buf, size_t size) {
	const size_t len = cx_strnlen(p_buf, size);
	*p_text_edit = (struct cx_text_edit) {
		.p_buf = p_buf,
		.buf_size = size,
		.len = len,
		.cursor_pos = len
	};
}

void cx_text_edit_insert(struct cx_text_edit* p_text_edit, const char* p_input, size_t n) {
	if (p_text_edit->len + n >= p_text_edit->buf_size) {
		n = p_text_edit->buf_size - p_text_edit->len - 1;
	}

	if (n == 0) {
		return;
	}

	if (p_text_edit->cursor_pos != p_text_edit->len) {
		const size_t dst_start = p_text_edit->cursor_pos + n;
		const size_t src_start = p_text_edit->cursor_pos;
		const size_t src_n = p_text_edit->len - src_start;
		memmove(p_text_edit->p_buf + dst_start, p_text_edit->p_buf + src_start, src_n);
	} else {
		p_text_edit->p_buf[p_text_edit->len + n] = '\0';
	}

	memcpy(p_text_edit->p_buf + p_text_edit->cursor_pos, p_input, n);

	p_text_edit->len += n;
	p_text_edit->cursor_pos += n;
}

static void cx_text_edit_delete_range(struct cx_text_edit* p_text_edit, size_t start, size_t n) {
	if (n == 0) {
		return;
	}

	if (start >= p_text_edit->len) {
		return;
	}

	if (start + n > p_text_edit->len) {
		n = p_text_edit->len - start;
	}

	void* p_dest = p_text_edit->p_buf + start;
	const void* p_src = (const uint8_t*)p_dest + n;
	const size_t move_len = (p_text_edit->len + 1) - (start + n);

	memmove(p_dest, p_src, move_len);

	p_text_edit->len -= n;
}

void cx_text_edit_delete(struct cx_text_edit* p_text_edit, int n) {
	size_t start = p_text_edit->cursor_pos;
	size_t n2 = (size_t)CX_M_ABS_INT32(n);

	if (n < 0) {
		if (n2 > p_text_edit->cursor_pos) {
			n2 = p_text_edit->cursor_pos;
		}
		start -= n2;
	}

	cx_text_edit_delete_range(p_text_edit, start, n2);

	p_text_edit->cursor_pos = start;
}

void cx_text_edit_clear(struct cx_text_edit* p_text_edit) {
	p_text_edit->len = 0;
	p_text_edit->cursor_pos = 0;
	p_text_edit->p_buf[0] = '\0';
}

void cx_text_edit_cursor_offset(struct cx_text_edit* p_text_edit, int offset) {
	int cursor = (int)p_text_edit->cursor_pos + offset;
	if (cursor < 0) {
		cursor = 0;
	}
	cx_text_edit_cursor_set(p_text_edit, (size_t)cursor);
}

void cx_text_edit_cursor_set(struct cx_text_edit* p_text_edit, size_t cursor) {
	if (cursor > p_text_edit->len) {
		cursor = p_text_edit->len;
	}
	p_text_edit->cursor_pos = cursor;
}

void cx_text_edit_cursor_next_word(struct cx_text_edit* p_text_edit) {
	if (p_text_edit->cursor_pos == p_text_edit->len) {
		return;
	}

	for (; p_text_edit->cursor_pos <= p_text_edit->len; ++p_text_edit->cursor_pos) {
		const char* p_0 = p_text_edit->p_buf + p_text_edit->cursor_pos;
		const char* p_1 = p_text_edit->p_buf + p_text_edit->cursor_pos + 1;
		if (isspace((unsigned char)*p_0) && !isspace((unsigned char)*p_1)) {
			++p_text_edit->cursor_pos;
			break;
		}
	}
}

void cx_text_edit_cursor_prev_word(struct cx_text_edit* p_text_edit) {
	if (p_text_edit->cursor_pos == 0) {
		return;
	}

	--p_text_edit->cursor_pos;

	for (; p_text_edit->cursor_pos > 0; --p_text_edit->cursor_pos) {
		const char* p_0 = p_text_edit->p_buf + p_text_edit->cursor_pos;
		const char* p_1 = p_text_edit->p_buf + p_text_edit->cursor_pos - 1;
		if (!isspace((unsigned char)*p_0) && isspace((unsigned char)*p_1)) {
			break;
		}
	}
}
