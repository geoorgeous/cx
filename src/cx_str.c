#include "cx_str.h"

int cx_strcmp_n(const char* s_a, const char* p_b, size_t len) {
	while(len-- && *s_a && *s_a == *p_b) {
		s_a++;
		p_b++;
	}

	return (unsigned char)(*s_a) - (unsigned char)((len != ((size_t)-1)) ? *p_b : '\0');
}
