#ifndef CX_STREAM_H
#define CX_STREAM_H

#include <stddef.h>

struct cx_stream_writer;

typedef int(*cx_stream_writer_write_fn)(struct cx_stream_writer*, size_t, const void*);

struct cx_stream_writer {
	cx_stream_writer_write_fn f_write;
};

static inline int cx_stream_write(struct cx_stream_writer* p_writer, size_t size, const void* p_bytes) {
	return p_writer->f_write(p_writer, size, p_bytes);
}

struct cx_stream_reader;

typedef int(*cx_stream_reader_read_fn)(struct cx_stream_reader*, size_t, void*);

struct cx_stream_reader {
	cx_stream_reader_read_fn f_read;
};

static inline int cx_stream_read(struct cx_stream_reader* p_reader, size_t size, void* p_bytes) {
	return p_reader->f_read(p_reader, size, p_bytes);
}

#endif
