#ifndef _H__PLATFORM
#define _H__PLATFORM

#if defined(_WIN32)
#define PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define PLATFORM_LINUX 1
#else
#define PLATFORM_WINDOWS 1
#endif

#if PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#endif

#endif