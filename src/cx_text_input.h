#ifndef CX_TEXT_INPUT_H

struct cx_text_input {
	char*        p_buffer;
	unsigned int buffer_size;
	unsigned int input_len;
	int          caret_pos;
};

void cx_text_input_init(struct cx_text_input* p_text_input, unsigned int max_input_len);
void cx_text_input_free(struct cx_text_input* p_text_input);
void cx_text_input_clear(struct cx_text_input* p_text_input);
void cx_text_input_insert(struct cx_text_input* p_text_input, const char* s);
void cx_text_input_delete(struct cx_text_input* p_text_input, int n);

#endif
