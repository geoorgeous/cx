#include <string.h>

#include "cx_macro.h"
#include "cx_stream_buffer.h"

static int cx_stream_write_buffer(struct cx_stream_writer* p_writer, size_t size, const void* p_bytes);
static int cx_stream_read_buffer(struct cx_stream_reader* p_reader, size_t size, void* p_bytes);

void cx_stream_writer_init_buffer(void* p_buffer, size_t capacity, struct cx_stream_writer_buffer* p_out) {
	*p_out = (struct cx_stream_writer_buffer) {
		.base = {
			.f_write = cx_stream_write_buffer
		},
		.p_buffer = p_buffer,
		.capacity = capacity
	};
}

void cx_stream_reader_init_buffer(const void* p_buffer, size_t capacity, struct cx_stream_reader_buffer* p_out) {
	*p_out = (struct cx_stream_reader_buffer) {
		.base = {
			.f_read = cx_stream_read_buffer
		},
		.p_buffer = p_buffer,
		.capacity = capacity
	};
}

int cx_stream_write_buffer(struct cx_stream_writer* p_writer, size_t size, const void* p_bytes) {
	struct cx_stream_writer_buffer* p_writer_buffer = (void*)p_writer;
	
	void* p_dst = (char*)p_writer_buffer->p_buffer + p_writer_buffer->position;

	p_writer_buffer->position += size;

	if (p_writer_buffer->position > p_writer_buffer->capacity) {
		return CX_FALSE;
	}

	memcpy(p_dst, p_bytes, size);
	return CX_TRUE;
}

int cx_stream_read_buffer(struct cx_stream_reader* p_reader, size_t size, void* p_bytes) {
	struct cx_stream_reader_buffer* p_reader_buffer = (void*)p_reader;

	const void* p_src = (const char*)p_reader_buffer->p_buffer + p_reader_buffer->position;

	p_reader_buffer->position += size;

	if (p_reader_buffer->position > p_reader_buffer->capacity) {
		return CX_FALSE;
	}
	
	memcpy(p_bytes, p_src, size);
	return CX_TRUE;
}
