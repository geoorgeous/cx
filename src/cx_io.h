#ifndef CX_IO_H
#define CX_IO_H

#include <stddef.h>

#include "cx_error.h"
#include "cx_macro.h"
#include "cx_result.h"

#define CX_LOG_CAT_IO "io"

enum cx_error cx_io_file_read_all(const char* s_filepath, void** pp_out_buf, size_t* p_out_size);

void cx_io_file_free(void* p_file_read_all_result);

void cx_io_filepath_dir(const char* s_filepath, size_t* p_out_dir_len);
void cx_io_filepath_stem(const char* s_filepath, const char** pp_out_stem_start, size_t* p_out_stem_len);
void cx_io_filepath_stem_cpy(const char* s_filepath, char* s_out, size_t* p_out_len);
void cx_io_filepath_join(const char* s_a, const char* s_b, char* s_out);
int  cx_io_filepath_ext(const char* s_filepath, const char** pp_out_ext_start, size_t* p_out_ext_len);
int  cx_io_filepath_exists(const char* s_filepath);
int  cx_io_filepath_is_file(const char* s_filepath);
int  cx_io_filepath_is_dir(const char* s_filepath);

struct cx_io_dir {
	const char* s_dir;
	CX_OPAQUE_INTERNALS(8);
};

enum cx_io_dir_entry_type {
	CX_IO_DIR_ENTRY_TYPE_EOD, // End of directory
	CX_IO_DIR_ENTRY_TYPE_FILE,
	CX_IO_DIR_ENTRY_TYPE_DIR
};

struct cx_io_dir_entry {
	enum cx_io_dir_entry_type type;
	const struct cx_io_dir* p_dir;
	const char* s_name;
	CX_OPAQUE_INTERNALS(8);
};

typedef int(*cx_io_dir_enumerate_cb_fn)(struct cx_io_dir_entry*, void*);

cx_result cx_io_dir_open(const char* s_dir, struct cx_io_dir* p_out);
cx_result cx_io_dir_next_entry(struct cx_io_dir* p_dir, struct cx_io_dir_entry* p_out);
void cx_io_dir_close(struct cx_io_dir* p_dir);
void cx_io_dir_enumerate(const char* s_dir, cx_io_dir_enumerate_cb_fn f_cb, void* p_user_ptr);
void cx_io_dir_enumerate_recursive(const char* s_dir, cx_io_dir_enumerate_cb_fn f_cb, void* p_user_ptr);

#endif
