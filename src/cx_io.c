#include <stdio.h>
#include <stdlib.h>

#include "cx_io.h"
#include "cx_error.h"
#include "cx_logging.h"

enum cx_error cx_io_file_read_all(const char* s_filename, void** pp_out_buf, size_t* p_out_size) {
	CX_LOG_FMT(INFO, IO, "Reading file from disk '%s'...\n", s_filename);

	FILE* file = fopen(s_filename, "rb");

	*pp_out_buf = 0;
	*p_out_size = 0;

	if (!file) {
		return CX_ERROR_io;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return CX_ERROR_io;
	}

	long size = ftell(file);

	if (size < 0) {
		fclose(file);
		return CX_ERROR_io;
	}
	
	rewind(file);

	void* p_buf = malloc((size_t)size + 1);

	if (!p_buf) {
		fclose(file);
		return CX_ERROR_allocation_failed;
	}

	size_t read = fread(p_buf, 1, (size_t)size, file);
	fclose(file);

	if (read < (size_t)size) {
		free(p_buf);
		return CX_ERROR_io;
	}

	((char*)p_buf)[size] = '\0';

	*pp_out_buf = p_buf;
	*p_out_size = (size_t)size;

	return CX_ERROR_none;
}

void cx_io_file_free(void* p_file_read_all_result) {
	free(p_file_read_all_result);
}
