#include "cx_logging.h"
#include "cx_stream_file.h"

static int cx_stream_file_read(struct cx_stream* p_stream, size_t size, void* p_bytes);
static int cx_stream_file_write(struct cx_stream* p_stream, size_t size, const void* p_bytes);
static size_t cx_stream_file_tell(const struct cx_stream* p_stream);
static int cx_stream_file_seek(struct cx_stream* p_stream, ptrdiff_t offset, enum cx_stream_seek_origin origin);

void cx_stream_file_init(FILE* p_file, struct cx_stream_file* p_out) {
	*p_out = (struct cx_stream_file) {
		.base = {
			.f_read_ = cx_stream_file_read,
			.f_write_ = cx_stream_file_write,
			.f_tell_ = cx_stream_file_tell,
			.f_seek_ = cx_stream_file_seek
		},
		.p_file = p_file
	};
}

int cx_stream_file_open(const char* s_filename, const char* s_mode, struct cx_stream_file* p_out) {
	FILE* p_file = fopen(s_filename, s_mode);

	if (!p_file) {
		CX_LOG_FMT(ERROR, STREAM, "Failed to open create stream from file '%s'\n", s_filename);
		return CX_FALSE;
	}
	
	cx_stream_file_init(p_file, p_out);
	return CX_TRUE;
}

void cx_stream_file_close(struct cx_stream_file* p_stream) {
	fclose(p_stream->p_file);
	p_stream->p_file = CX_NULL;
}

int cx_stream_file_read(struct cx_stream* p_stream, size_t size, void* p_bytes) {
	return fread(p_bytes, 1, size, ((struct cx_stream_file*)p_stream)->p_file) == size;
}

int cx_stream_file_write(struct cx_stream* p_stream, size_t size, const void* p_bytes) {
	return fwrite(p_bytes, 1, size, ((struct cx_stream_file*)p_stream)->p_file) == size;
}

size_t cx_stream_file_tell(const struct cx_stream* p_stream) {
	const long retval = ftell(((const struct cx_stream_file*)p_stream)->p_file);
	return retval < 0 ? 0 : (size_t)retval;
}

int cx_stream_file_seek(struct cx_stream* p_stream, ptrdiff_t offset, enum cx_stream_seek_origin origin) {
	int whence =
		origin == CX_STREAM_SEEK_ORIGIN_begin ? SEEK_SET :
		origin == CX_STREAM_SEEK_ORIGIN_current ? SEEK_CUR :
		SEEK_END;
	const long retval = fseek(((struct cx_stream_file*)p_stream)->p_file, offset, whence);
	return retval == 0;
}
