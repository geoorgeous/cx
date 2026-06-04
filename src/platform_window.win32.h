#ifndef PLATFORM_WINDOW_WIN32_H
#define PLATFORM_WINDOW_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

struct platform_window_win32_internals {
	HWND hwnd;
	HDC  hdc;
};

#endif
