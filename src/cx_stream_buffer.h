#ifndef CX_STREAM_BUFFER_H
#define CX_STREAM_BUFFER_H

#include <stddef.h>

#include "cx_stream.h"

struct cx_stream_writer_buffer {
	struct cx_stream_writer base;
	void* p_buffer;
	size_t position;
	size_t capacity;
};

void cx_stream_writer_init_buffer(void* p_buffer, size_t capacity, struct cx_stream_writer_buffer* p_out);

struct cx_stream_reader_buffer {
	struct cx_stream_reader base;
	const void* p_buffer;
	size_t position;
	size_t capacity;
};

void cx_stream_reader_init_buffer(const void* p_buffer, size_t capacity, struct cx_stream_reader_buffer* p_out);

#endif
