#ifndef CX_BITS_H
#define CX_BITS_H

#include <stddef.h>
#include <stdint.h>

#define CX_BIT_GET(BYTE, N) (((BYTE) >> (uint32_t)(N)) & 1u)

#define CX_BIT_SET(BYTE, N, X) (((BYTE) & ~(1u << (uint32_t)(N))) | ((uint32_t)(X) << (uint32_t)(N)))

void cx_bits_copy(uint8_t* p_dst, size_t dst_bit, const uint8_t* p_src, size_t src_bit, size_t n);

#endif
