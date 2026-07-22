#ifndef CX_STREAM_H
#define CX_STREAM_H

#include <stddef.h>

#include "cx_dbg.h"

#define CX_LOG_CAT_STREAM "stream"

struct cx_stream;

enum cx_stream_seek_origin {
	CX_STREAM_SEEK_ORIGIN_begin,
	CX_STREAM_SEEK_ORIGIN_current,
	CX_STREAM_SEEK_ORIGIN_end
};

typedef int(*cx_stream_read_fn)(struct cx_stream*, size_t, void*);
typedef int(*cx_stream_write_fn)(struct cx_stream*, size_t, const void*);
typedef size_t(*cx_stream_tell_fn)(const struct cx_stream*);
typedef int(*cx_stream_seek_fn)(struct cx_stream*, ptrdiff_t, enum cx_stream_seek_origin);
typedef void(*cx_stream_close_fn)(struct cx_stream*);

struct cx_stream {
	cx_stream_read_fn f_read_;
	cx_stream_write_fn f_write_;
	cx_stream_tell_fn f_tell_;
	cx_stream_seek_fn f_seek_;
	cx_stream_close_fn f_close_;
};

static inline int cx_stream_read(struct cx_stream* p_stream, size_t size, void* p_bytes) {
	CX_ASSERT_MSG(p_stream->f_read_, STREAM, "Stream does not support 'read'\n");
	return p_stream->f_read_(p_stream, size, p_bytes);
}

static inline int cx_stream_write(struct cx_stream* p_stream, size_t size, const void* p_bytes) {
	CX_ASSERT_MSG(p_stream->f_write_, STREAM, "Stream does not support 'write'\n");
	return p_stream->f_write_(p_stream, size, p_bytes);
}

static inline size_t cx_stream_tell(const struct cx_stream* p_stream) {
	CX_ASSERT_MSG(p_stream->f_tell_, STREAM, "Stream does not support 'tell'\n");
	return p_stream->f_tell_(p_stream);
}

static inline int cx_stream_seek(struct cx_stream* p_stream, ptrdiff_t offset, enum cx_stream_seek_origin origin) {
	CX_ASSERT_MSG(p_stream->f_seek_, STREAM, "Stream does not support 'seek'\n");
	return p_stream->f_seek_(p_stream, offset, origin);
}

static inline void cx_stream_close(struct cx_stream* p_stream) {
	CX_ASSERT_MSG(p_stream->f_close_, STREAM, "Stream does not support 'close'\n");
	p_stream->f_close_(p_stream);
}

#endif
