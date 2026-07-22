#include <string.h>

#include "cx_macro.h"
#include "cx_stream_buffer.h"

static int cx_stream_buffer_read(struct cx_stream* p_stream, size_t size, void* p_bytes);
static int cx_stream_buffer_write(struct cx_stream* p_stream, size_t size, const void* p_bytes);
static size_t cx_stream_buffer_tell(const struct cx_stream* p_stream);
static int cx_stream_buffer_seek(struct cx_stream* p_stream, ptrdiff_t offset, enum cx_stream_seek_origin origin);
static void cx_stream_buffer_close(struct cx_stream* p_stream) { (void)p_stream; }

void cx_stream_buffer_init(void* p_buffer, size_t capacity, struct cx_stream_buffer* p_out) {
	*p_out = (struct cx_stream_buffer) {
		.base = {
			.f_read_ = cx_stream_buffer_read,
			.f_write_ = cx_stream_buffer_write,
			.f_tell_ = cx_stream_buffer_tell,
			.f_seek_ = cx_stream_buffer_seek,
			.f_close_ = cx_stream_buffer_close
		},
		.p_buffer = p_buffer,
		.capacity = capacity
	};
}

int cx_stream_buffer_read(struct cx_stream* p_stream, size_t size, void* p_bytes) {
	struct cx_stream_buffer* p_stream_buffer = (void*)p_stream;

	const void* p_src = (const char*)p_stream_buffer->p_buffer + p_stream_buffer->position;

	p_stream_buffer->position += size;

	if (p_stream_buffer->position > p_stream_buffer->capacity) {
		return CX_FALSE;
	}
	
	memcpy(p_bytes, p_src, size);
	return CX_TRUE;
}

int cx_stream_buffer_write(struct cx_stream* p_stream, size_t size, const void* p_bytes) {
	struct cx_stream_buffer* p_stream_buffer = (void*)p_stream;
	
	void* p_dst = (char*)p_stream_buffer->p_buffer + p_stream_buffer->position;

	p_stream_buffer->position += size;

	if (p_stream_buffer->position > p_stream_buffer->capacity) {
		return CX_FALSE;
	}

	memcpy(p_dst, p_bytes, size);
	return CX_TRUE;
}

size_t cx_stream_buffer_tell(const struct cx_stream* p_stream) {
	return ((const struct cx_stream_buffer*)p_stream)->position;
}

int cx_stream_buffer_seek(struct cx_stream* p_stream, ptrdiff_t offset, enum cx_stream_seek_origin origin) {
	struct cx_stream_buffer* p_stream_buffer = (void*)p_stream;
	
	size_t new_position;

	if (origin == CX_STREAM_SEEK_ORIGIN_current) {
		new_position = (size_t)((ptrdiff_t)p_stream_buffer->position + offset);
	} else if (origin == CX_STREAM_SEEK_ORIGIN_end) {
		new_position = (size_t)((ptrdiff_t)p_stream_buffer->capacity + offset);
	} else {
		CX_ASSERT_MSG(offset >= 0, STREAM, "Attempted to seek before the beginning of the stream\n");
		new_position = (size_t)offset;
	}

	if (new_position > p_stream_buffer->capacity) {
		return CX_FALSE;
	}

	p_stream_buffer->position = new_position;
	
	return CX_TRUE;
}
