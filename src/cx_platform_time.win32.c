#include "cx_platform_time.h"
#include <stdint.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

uint64_t cx_platform_time_now(void) {
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (uint64_t)counter.QuadPart;
}

uint64_t cx_platform_time_frequency(void) {
	static uint64_t freq = 0;

	if (freq) {
		return freq;
	}

	LARGE_INTEGER counter_freq;
	QueryPerformanceFrequency(&counter_freq);
	freq = (uint64_t)counter_freq.QuadPart;
	return freq;
}

double cx_platform_time_delta_seconds(uint64_t start, uint64_t end) {
	return (double)(end - start) / (double)cx_platform_time_frequency();
}

double cx_platform_time_delta_milliseconds(uint64_t start, uint64_t end) {
	return ((double)(end - start) * 1e3) / (double)cx_platform_time_frequency();
}

double cx_platform_time_delta_microseconds(uint64_t start, uint64_t end) {
	return ((double)(end - start) * 1e6) / (double)cx_platform_time_frequency();
}
