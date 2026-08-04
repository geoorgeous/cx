#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cx_io.h"
#include "cx_error.h"
#include "cx_logging.h"

enum cx_error cx_io_file_read_all(const char* s_filename, void** pp_out_buf, size_t* p_out_size) {
	CX_LOG_FMT(INFO, IO, "Reading file from disk '%s'...\n", s_filename);

	FILE* file = fopen(s_filename, "rb");

	*pp_out_buf = 0;

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

	if (p_out_size) {
		*p_out_size = (size_t)size;
	}

	return CX_ERROR_none;
}

void cx_io_file_free(void* p_file_read_all_result) {
	free(p_file_read_all_result);
}

void cx_io_filepath_stem(const char* s_filepath, const char** pp_out_stem_start, size_t* p_out_stem_len) {
	const char* p_fwd = strrchr(s_filepath, '/');
	const char* p_bwd = strrchr(s_filepath, '\\');
	const char* p_sep = (p_fwd > p_bwd) ? p_fwd : p_bwd;

	*pp_out_stem_start = p_sep ? (p_sep + 1) : s_filepath;

	if ((*pp_out_stem_start)[0] == '\0') {
		*p_out_stem_len = 0;
		return;
	}

	const char* p_dot = strrchr(*pp_out_stem_start, '.');

	*p_out_stem_len =
		p_dot && p_dot != *pp_out_stem_start ?
		(size_t)(p_dot - *pp_out_stem_start) :
		strlen(*pp_out_stem_start);
}

void cx_io_filepath_stem_cpy(const char* s_filename, char* s_out, size_t* p_out_len) {
	const char* p_filepath_stem;
	cx_io_filepath_stem(s_filename, &p_filepath_stem, p_out_len);

	if (!s_out) {
		return;
	}
	
	strncpy(s_out, p_filepath_stem, *p_out_len);
}
