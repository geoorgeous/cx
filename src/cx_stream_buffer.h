#ifndef CX_STREAM_BUFFER_H
#define CX_STREAM_BUFFER_H

#include <stddef.h>

#include "cx_stream.h"

struct cx_stream_buffer {
	struct cx_stream base;
	void* p_buffer;
	size_t position;
	size_t capacity;
};

void cx_stream_buffer_init(void* p_buffer, size_t capacity, struct cx_stream_buffer* p_out);

#endif
