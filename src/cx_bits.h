#ifndef CX_BITS_H
#define CX_BITS_H

#include <stddef.h>
#include <stdint.h>

#define CX_BIT_GET(BYTE, N) (((BYTE) >> (N)) & 1u)

#define CX_BIT_SET(BYTE, N, X) (((BYTE) & ~(1u << (N))) | ((X) << (N)))

void cx_bits_copy(uint8_t* p_dst, size_t dst_bit, const uint8_t* p_src, size_t src_bit, size_t n);

#endif
