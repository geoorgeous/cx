#ifndef _H__ERRORS
#define _H__ERRORS

enum error {
    ERROR_none = 0,

	ERROR_invalid_argument,
	ERROR_allocation_failed,
	ERROR_index_out_of_range,
	ERROR_not_found,
	ERROR_not_supported,
	ERROR_invalid_state,

	ERROR_api_x11,
	ERROR_api_win32,
	ERROR_api_glx,
	ERROR_api_wgl,

	ERROR_gfx_program_build_failure
};

#endif
