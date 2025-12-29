#ifndef _H__SERIALIZATION
#define _H__SERIALIZATION

#include <stdio.h>
#include <stdint.h>

void serialize_bytes(FILE* p_file, const void* p_dat, size_t size);
void serialize_uint8(FILE* p_file, uint8_t dat);
void serialize_uint16(FILE* p_file, uint16_t dat);
void serialize_uint32(FILE* p_file, uint32_t dat);
void serialize_int8(FILE* p_file, int8_t dat);
void serialize_int16(FILE* p_file, int16_t dat);
void serialize_int32(FILE* p_file, int32_t dat);
void serialize_size(FILE* p_file, size_t size);
void serialize_str(FILE* p_file, const char* s_str, uint32_t len);

void deserialize_bytes(FILE* p_file, void* p_dst, size_t size);
void deserialize_uint8(FILE* p_file, uint8_t* p_result);
void deserialize_uint16(FILE* p_file, uint16_t* p_result);
void deserialize_uint32(FILE* p_file, uint32_t* p_result);
void deserialize_int8(FILE* p_file, int8_t* p_result);
void deserialize_int16(FILE* p_file, int16_t* p_result);
void deserialize_int32(FILE* p_file, int32_t* p_result);
void deserialize_size(FILE* p_file, size_t* p_result);
void deserialize_str(FILE* p_file, char* s_dst, uint32_t* p_result_len);

#endif