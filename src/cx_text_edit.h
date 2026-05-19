#ifndef CX_TEXT_EDIT_H
#define CX_TEXT_EDIT_H

#include <stddef.h>

struct cx_text_edit {
	char* p_buf;
	size_t buf_size;
	size_t len;
	size_t cursor_pos;
};

void cx_text_edit_insert(struct cx_text_edit* p_text_edit, const char* p_input, size_t n);
void cx_text_edit_delete(struct cx_text_edit* p_text_edit, int n);
void cx_text_edit_clear(struct cx_text_edit* p_text_edit);
void cx_text_edit_cursor_offset(struct cx_text_edit* p_text_edit, int offset);
void cx_text_edit_cursor_set(struct cx_text_edit* p_text_edit, size_t cursor);
void cx_text_edit_cursor_next_word(struct cx_text_edit* p_text_edit);
void cx_text_edit_cursor_prev_word(struct cx_text_edit* p_text_edit);

#endif
