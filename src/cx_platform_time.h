#ifndef CX_PLATFORM_TIME_H
#define CX_PLATFORM_TIME_H

#include <stdint.h>

uint64_t cx_platform_time_now(void);
uint64_t cx_platform_time_frequency(void);
double cx_platform_time_delta_seconds(uint64_t start, uint64_t end);
double cx_platform_time_delta_milliseconds(uint64_t start, uint64_t end);
double cx_platform_time_delta_microseconds(uint64_t start, uint64_t end);

#endif
