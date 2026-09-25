#include <stdint.h>

#include "cx_str.h"

char* cx_str_f32(char* p_dst, float f) {
	if (f < 0.0f) {
		*p_dst++ = '-';
		f = -f;
	}

	int32_t integer_part = (int32_t)f;
	float decimal_part = f - (float)integer_part;
	int32_t decimal_part_fixed_point = (int32_t)(decimal_part * 1000.0f + 0.5f);

	char buf[12];
	int n = 0;

	do {
		buf[n++] = (char)('0' + (integer_part % 10));
		integer_part /= 10;
	} while(integer_part);

	while (n--) {
		*p_dst++ = buf[n];
	}

	*p_dst++ = '.';
	*p_dst++ = (char)('0' + (decimal_part_fixed_point / 100) % 10);
	*p_dst++ = (char)('0' + (decimal_part_fixed_point /  10) % 10);
	*p_dst++ = (char)('0' + (decimal_part_fixed_point /   1) % 10);

	return p_dst;
}

char* cx_str_f32_n(char* p_dst, const float* p_v, size_t n) {
	p_dst = cx_str_f32(p_dst, p_v[0]);

	for (size_t i = 1; i < n; ++i) {
		*p_dst++ = ',';
		*p_dst++ = ' ';
		p_dst = cx_str_f32(p_dst, p_v[i]);
	}

	return p_dst;
}

uint32_t cx_str_hash(const char* s_str) {
	// FNV-1a hash algorithm 

	const uint32_t offset_basis = 2166136261;
	const uint32_t fnv_prime = 16777619;

	uint32_t result = offset_basis;

	for(; *s_str; s_str++) {
		result ^= (uint8_t)*s_str;
		result *= fnv_prime;
	}

	return result;
}
