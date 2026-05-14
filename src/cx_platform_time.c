#include "cx_platform_time.h"

#ifdef PLATFORM_WIN32
#include "cx_platform_time.win32.c"
#else
#include "cx_platform_time.nix.c"
#endif
