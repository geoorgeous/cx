#include "cx_bits.h"

void cx_bits_copy(uint8_t* p_dst, size_t dst_bit, const uint8_t* p_src, size_t src_bit, size_t n) {
	for (size_t i = 0; i < n; ++i) {
		const size_t src_bit_n = src_bit + i;
		const uint8_t src_byte = p_src[src_bit_n / 8];
		const uint8_t src_bit_val = CX_BIT_GET(src_byte, src_bit_n % 8);

		const size_t dst_bit_n = dst_bit + i;
		uint8_t* dst_byte = p_dst + (dst_bit_n / 8);
		*dst_byte = (uint8_t)CX_BIT_SET(*dst_byte, dst_bit_n % 8, src_bit_val);
	}
}
