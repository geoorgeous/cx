#ifndef CX_HASH_H
#define CX_HASH_H

#include <stddef.h>
#include <stdint.h>

#define CX_HASH_FNV_OFFSET 1469598103934665603ull
#define CX_HASH_FNV_PRIME  1099511628211ull

static inline uint64_t cx_hash_init(void) {
	return CX_HASH_FNV_OFFSET;
}

static inline uint64_t cx_hash_bytes(uint64_t hash, const void* p, size_t n) {
	for (size_t i = 0; i < n; ++i) {
		hash ^= ((const uint8_t*)p)[i];
		hash *= CX_HASH_FNV_PRIME;
	}
	return hash;
}

static inline uint64_t cx_hash_u8(uint64_t hash, uint8_t u8) {
	return cx_hash_bytes(hash, &u8, sizeof(u8));
}

static inline uint64_t cx_hash_u16(uint64_t hash, uint16_t u16) {
	return cx_hash_bytes(hash, &u16, sizeof(u16));
}

static inline uint64_t cx_hash_u32(uint64_t hash, uint32_t u32) {
	return cx_hash_bytes(hash, &u32, sizeof(u32));
}

static inline uint64_t cx_hash_u64(uint64_t hash, uint64_t u64) {
	return cx_hash_bytes(hash, &u64, sizeof(u64));
}

static inline uint64_t cx_hash_i8(uint64_t hash, int8_t i8) {
	return cx_hash_bytes(hash, &i8, sizeof(i8));
}

static inline uint64_t cx_hash_i16(uint64_t hash, int16_t i16) {
	return cx_hash_bytes(hash, &i16, sizeof(i16));
}

static inline uint64_t cx_hash_i32(uint64_t hash, int32_t i32) {
	return cx_hash_bytes(hash, &i32, sizeof(i32));
}

static inline uint64_t cx_hash_i64(uint64_t hash, int64_t i64) {
	return cx_hash_bytes(hash, &i64, sizeof(i64));
}

static inline uint64_t cx_hash_f32(uint64_t hash, float f32) {
	return cx_hash_bytes(hash, &f32, sizeof(f32));
}

static inline uint64_t cx_hash_f64(uint64_t hash, double f64) {
	return cx_hash_bytes(hash, &f64, sizeof(f64));
}

#endif
