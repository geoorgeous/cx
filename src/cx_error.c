#include "cx_error.h"

const char* cx_error_get_string(enum cx_error error) {
	static const char* error_strings[] = {
		0,
		"Invalid argument",
		"Allocation failed",
		"Index out of range",
		"Not found",
		"Not supported",
		"Invalid state",
		"X11",
		"Win32",
		"glX",
		"Wgl",
		"Graphics program failed to be built",
		"IO"
	};
	return error_strings[error];
}
