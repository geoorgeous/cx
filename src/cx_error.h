#ifndef CX_ERROR_H
#define CX_ERROR_H

enum cx_error {
    CX_ERROR_none = 0,

	CX_ERROR_invalid_argument,
	CX_ERROR_allocation_failed,
	CX_ERROR_index_out_of_range,
	CX_ERROR_not_found,
	CX_ERROR_not_supported,
	CX_ERROR_invalid_state,

	CX_ERROR_api_x11,
	CX_ERROR_api_win32,
	CX_ERROR_api_glx,
	CX_ERROR_api_wgl,

	CX_ERROR_gfx_program_build_failure,

	CX_ERROR_io,
};

#endif
