#ifndef CX_STR_H
#define CX_STR_H

#include <stddef.h>

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

#endif
