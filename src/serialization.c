#include <string.h>

#include "serialization.h"

void serialize_bytes(FILE* p_file, const void* p_dat, size_t size) {
    fwrite(p_dat, size, 1, p_file);
}

void serialize_uint8(FILE* p_file, uint8_t dat) {
    serialize_bytes(p_file, &dat, sizeof(dat));
}

void serialize_uint16(FILE* p_file, uint16_t dat) {
    serialize_bytes(p_file, &dat, sizeof(dat));
}

void serialize_uint32(FILE* p_file, uint32_t dat) {
    serialize_bytes(p_file, &dat, sizeof(dat));
}

void serialize_int8(FILE* p_file, int8_t dat) {
    serialize_bytes(p_file, &dat, sizeof(dat));
}

void serialize_int16(FILE* p_file, int16_t dat) {
    serialize_bytes(p_file, &dat, sizeof(dat));
}

void serialize_int32(FILE* p_file, int32_t dat) {
    serialize_bytes(p_file, &dat, sizeof(dat));
}

void serialize_size(FILE* p_file, size_t size) {
    serialize_bytes(p_file, &size, sizeof(size));
}

void serialize_str(FILE* p_file, const char* s_str, uint32_t len) {
    if (!len)
        len = strlen(s_str);
    serialize_uint32(p_file, len);
    serialize_bytes(p_file, s_str, len);
}

void deserialize_bytes(FILE* p_file, void* p_dst, size_t size) {
    fread(p_dst, size, 1, p_file);
}

void deserialize_uint8(FILE* p_file, uint8_t* p_result) {
    deserialize_bytes(p_file, p_result, sizeof(*p_result));
}

void deserialize_uint16(FILE* p_file, uint16_t* p_result) {
    deserialize_bytes(p_file, p_result, sizeof(*p_result));
}

void deserialize_uint32(FILE* p_file, uint32_t* p_result) {
    deserialize_bytes(p_file, p_result, sizeof(*p_result));
}

void deserialize_int8(FILE* p_file, int8_t* p_result) {
    deserialize_bytes(p_file, p_result, sizeof(*p_result));
}

void deserialize_int16(FILE* p_file, int16_t* p_result) {
    deserialize_bytes(p_file, p_result, sizeof(*p_result));
}

void deserialize_int32(FILE* p_file, int32_t* p_result) {
    deserialize_bytes(p_file, p_result, sizeof(*p_result));
}

void deserialize_size(FILE* p_file, size_t* p_result) {
    deserialize_bytes(p_file, p_result, sizeof(*p_result));
}

void deserialize_str(FILE* p_file, char* s_dst, uint32_t* p_result_len) {
    if (!*p_result_len) {
        deserialize_uint32(p_file, p_result_len);
        return;
    }
    deserialize_bytes(p_file, s_dst, *p_result_len);
    s_dst[*p_result_len] = '\0';
}