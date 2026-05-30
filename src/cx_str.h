#ifndef CX_STR_H
#define CX_STR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cx_alloc.h"

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

#endif
