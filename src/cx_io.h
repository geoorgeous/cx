#ifndef CX_IO_H
#define CX_IO_H

#include <stddef.h>

#include "cx_error.h"

#define CX_LOG_CAT_IO "io"

enum cx_error cx_io_file_read_all(const char* s_filepath, void** pp_out_buf, size_t* p_out_size);

void cx_io_file_free(void* p_file_read_all_result);

void cx_io_filepath_stem(const char* s_filepath, const char** pp_out_stem_start, size_t* p_out_stem_len);
void cx_io_filepath_stem_cpy(const char* s_filepath, char* s_out, size_t* p_out_len);
int  cx_io_filepath_ext(const char* s_filepath, const char** pp_out_ext_start, size_t* p_out_ext_len);
int  cx_io_filepath_exists(const char* s_filepath);
int  cx_io_filepath_is_file(const char* s_filepath);
int  cx_io_filepath_is_dir(const char* s_filepath);

#endif
