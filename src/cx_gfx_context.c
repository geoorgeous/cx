#include "cx_gfx_context.h"

#ifdef PLATFORM_WIN32
#include "cx_gfx_context.win32.c"
#else
#include "cx_gfx_context.x11.c"
#endif
