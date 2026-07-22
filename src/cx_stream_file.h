#ifndef CX_STREAM_FILE_H
#define CX_STREAM_FILE_H

#include <stdio.h>

#include "cx_stream.h"

struct cx_stream_file {
	struct cx_stream base;
	FILE* p_file;
};

void cx_stream_file_init(FILE* p_file, struct cx_stream_file* p_out);
int cx_stream_file_open(const char* s_filename, const char* s_mode, struct cx_stream_file* p_out);

#endif
