#ifndef CX_IO_H
#define CX_IO_H

#include <stddef.h>

#include "cx_error.h"

#define CX_LOG_CAT_IO "io"

enum cx_error cx_io_file_read_all(const char* s_filename, void** pp_out_buf, size_t* p_out_size);

void cx_io_file_free(void* p_file_read_all_result);

#endif
