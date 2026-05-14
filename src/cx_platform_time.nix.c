#include "cx_platform_time.h"

#include <stdint.h>
#include <time.h>

uint64_t cx_platform_time_now(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

uint64_t cx_platform_time_frequency(void) {
	return 1000000000ull;
}

double cx_platform_time_delta_seconds(uint64_t start, uint64_t end) {
	return (double)(end - start) / 1e9;
}

double cx_platform_time_delta_milliseconds(uint64_t start, uint64_t end) {
	return (double)(end - start) / 1e6;
}

double cx_platform_time_delta_microseconds(uint64_t start, uint64_t end) {
	return (double)(end - start) / 1e3;
}
