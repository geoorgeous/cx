#include "gl_context.h"

#ifdef PLATFORM_WIN32
#include "gl_context_win32.c"
#else
#include "gl_context_nix_x11.c"
#endif