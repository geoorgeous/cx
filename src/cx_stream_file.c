#include "cx_stream_file.h"

static int cx_stream_write_file(struct cx_stream_writer* p_writer, size_t size, const void* p_bytes);
static int cx_stream_read_file(struct cx_stream_reader* p_reader, size_t size, void* p_bytes);

void cx_stream_writer_init_file(FILE* p_file, struct cx_stream_writer_file* p_out) {
	*p_out = (struct cx_stream_writer_file) {
		.base.f_write = cx_stream_write_file,
		.p_file = p_file
	};
}

void cx_stream_reader_init_file(FILE* p_file, struct cx_stream_reader_file* p_out) {
	*p_out = (struct cx_stream_reader_file) {
		.base.f_read = cx_stream_read_file,
		.p_file = p_file
	};
}

int cx_stream_write_file(struct cx_stream_writer* p_writer, size_t size, const void* p_bytes) {
	return fwrite(p_bytes, 1, size, ((struct cx_stream_writer_file*)p_writer)->p_file) == size;
}

int cx_stream_read_file(struct cx_stream_reader* p_reader, size_t size, void* p_bytes) {
	return fread(p_bytes, 1, size, ((struct cx_stream_reader_file*)p_reader)->p_file) == size;
}
