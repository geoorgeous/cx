#ifndef CX_STREAM_FILE_H
#define CX_STREAM_FILE_H

#include <stdio.h>

#include "cx_stream.h"

struct cx_stream_writer_file {
	struct cx_stream_writer base;
	FILE* p_file;
};

void cx_stream_writer_init_file(FILE* p_file, struct cx_stream_writer_file* p_out);

struct cx_stream_reader_file {
	struct cx_stream_reader base;
	FILE* p_file;
};

void cx_stream_reader_init_file(FILE* p_file, struct cx_stream_reader_file* p_out);

#endif
