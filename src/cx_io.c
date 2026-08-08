#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "cx_io.h"
#include "cx_error.h"
#include "cx_logging.h"
#include "cx_str.h"

enum cx_error cx_io_file_read_all(const char* s_filepath, void** pp_out_buf, size_t* p_out_size) {
	CX_LOG_FMT(INFO, IO, "Reading file from disk '%s'...\n", s_filepath);

	FILE* file = fopen(s_filepath, "rb");

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

void cx_io_filepath_dir(const char* s_filepath, size_t* p_out_dir_len) {
	const char* p_fwd = strrchr(s_filepath, '/');
	const char* p_bwd = strrchr(s_filepath, '\\');
	const char* p_sep = (p_fwd > p_bwd) ? p_fwd : p_bwd;

	if (p_sep == CX_NULL) {
		*p_out_dir_len = 0;
		return;
	}

	*p_out_dir_len = (size_t)(p_sep - s_filepath);
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

void cx_io_filepath_join(const char* s_a, const char* s_b, char* s_out) {
	char* p = cx_stpcpy(s_out, s_a);
	if (p != s_out && p[-1] != '/') {
		*p++ = '/';
	}
	strcpy(p, s_b);
}

int cx_io_filepath_ext(const char* s_filepath, const char** pp_out_ext_start, size_t* p_out_ext_len) {
	const char* p_dot = strrchr(s_filepath, '.');

	if (p_dot == CX_NULL || p_dot[1] == '\0') {
		return CX_FALSE;
	}

	*pp_out_ext_start = p_dot + 1;
	*p_out_ext_len = strlen(*pp_out_ext_start);

	return CX_TRUE;
}

int cx_io_filepath_exists(const char* s_filepath) {
	struct stat st;
	return stat(s_filepath, &st) == 0;
}

int cx_io_filepath_is_file(const char* s_filepath) {
	struct stat st;
	return stat(s_filepath, &st) == 0 && S_ISREG(st.st_mode);
}

int cx_io_filepath_is_dir(const char* s_filepath) {
	struct stat st;
	return stat(s_filepath, &st) == 0 && S_ISDIR(st.st_mode);
}

void cx_io_dir_enumerate(const char* s_dir, cx_io_dir_enumerate_cb_fn f_cb, void* p_user_ptr) {
	struct cx_io_dir dir;

	if (cx_io_dir_open(s_dir, &dir) != CX_SUCCESS) {
		return;
	}

	struct cx_io_dir_entry dir_entry;

	while(
		cx_io_dir_next_entry(&dir, &dir_entry) == CX_SUCCESS &&
		dir_entry.type != CX_IO_DIR_ENTRY_TYPE_end) {
		
		if (!f_cb(&dir_entry, p_user_ptr)) {
			break;
		}
	}
}

void cx_io_dir_enumerate_recursive(const char* s_dir, cx_io_dir_enumerate_cb_fn f_cb, void* p_user_ptr) {
	struct cx_io_dir dir;

	if (cx_io_dir_open(s_dir, &dir) != CX_SUCCESS) {
		return;
	}

	CX_LOG_FMT(INFO, IO, "Enumerating directory '%s'\n", s_dir);

	struct cx_io_dir_entry dir_entry;

	while(
		cx_io_dir_next_entry(&dir, &dir_entry) == CX_SUCCESS &&
		dir_entry.type != CX_IO_DIR_ENTRY_TYPE_end) {

		if (cx_str_eq(dir_entry.s_name, ".") || cx_str_eq(dir_entry.s_name, "..")) {
			continue;
		}

		f_cb(&dir_entry, p_user_ptr);

		if (dir_entry.type == CX_IO_DIR_ENTRY_TYPE_dir) {
			char dirpath[256];
			cx_io_filepath_join(s_dir, dir_entry.s_name, dirpath);

			cx_io_dir_enumerate_recursive(dirpath, f_cb, p_user_ptr);
		}
	}

	cx_io_dir_close(&dir);
}

#ifdef PLATFORM_WIN32
#include "cx_io.win32.c"
#else
#include "cx_io.posix.c"
#endif
