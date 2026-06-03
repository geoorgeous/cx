#ifndef CX_STR_H
#define CX_STR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cx_alloc.h"

char* cx_str_f32(char* p_dst, float f);

char* cx_str_f32_n(char* p_dst, const float* p_v, size_t n);

static inline int cx_strcmp_n(const char* s_a, const char* p_b, size_t len) {
	while(len-- && *s_a && *s_a == *p_b) {
		s_a++;
		p_b++;
	}

	return (unsigned char)(*s_a) - (unsigned char)((len != ((size_t)-1)) ? *p_b : '\0');
}

static inline char* cx_stpcpy(char* p_dst, const char* s_src) {
	while((*p_dst = *s_src)) {
		++p_dst, ++s_src;
	}
	return p_dst;
}

static inline size_t cx_strnlen(const char* s, size_t n) {
	size_t len = 0;
	for(; len < n && *s++; len++);
	return len;
}

static inline char* cx_strndup(const char* s, size_t n) {
	char* p = CX_MALLOC(n + 1);
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}

static inline char* cx_strdup(const char* s) {
	return cx_strndup(s, strlen(s));
}

static inline char* cx_str_tmp_buf(void) {
	static char bufs[8][64];
	static unsigned int idx;
	return bufs[idx++ & 7];
}

static inline char* cx_str_tmp_vec3(const float* p_v) {
	char* p_buf = cx_str_tmp_buf();
	*cx_str_f32_n(p_buf, p_v, 3) = '\0';
	return p_buf;
}

static inline char* cx_str_tmp_quaternion(const float* p_q) {
	char* p_buf = cx_str_tmp_buf();
	*cx_str_f32_n(p_buf, p_q, 4) = '\0';
	return p_buf;
}

#endif
