#ifndef CX_STREAM_SERIALIZATION_H
#define CX_STREAM_SERIALIZATION_H

#include <stddef.h>
#include <stdint.h>

#define CX_LOG_CAT_STREAM_WRITE "stream:write"
#define CX_LOG_CAT_STREAM_READ "stream:read"

struct cx_stream;

int cx_stream_serialize_bytes(struct cx_stream* p_stream, size_t size, const void* p_bytes);
int cx_stream_serialize_uint8(struct cx_stream* p_stream, uint8_t value);
int cx_stream_serialize_uint16(struct cx_stream* p_stream, uint16_t value);
int cx_stream_serialize_uint32(struct cx_stream* p_stream, uint32_t value);
int cx_stream_serialize_uint64(struct cx_stream* p_stream, uint64_t value);
int cx_stream_serialize_int8(struct cx_stream* p_stream, int8_t value);
int cx_stream_serialize_int16(struct cx_stream* p_stream, int16_t value);
int cx_stream_serialize_int32(struct cx_stream* p_stream, int32_t value);
int cx_stream_serialize_int64(struct cx_stream* p_stream, int64_t value);
int cx_stream_serialize_float32(struct cx_stream* p_stream, float value);
int cx_stream_serialize_float64(struct cx_stream* p_stream, double value);
int cx_stream_serialize_string(struct cx_stream* p_stream, const char* p_str, size_t len);

int cx_stream_deserialize_bytes(struct cx_stream* p_stream, size_t size, void* p_bytes);
int cx_stream_deserialize_uint8(struct cx_stream* p_stream, uint8_t* p_out);
int cx_stream_deserialize_uint16(struct cx_stream* p_stream, uint16_t* p_out);
int cx_stream_deserialize_uint32(struct cx_stream* p_stream, uint32_t* p_out);
int cx_stream_deserialize_uint64(struct cx_stream* p_stream, uint64_t* p_out);
int cx_stream_deserialize_int8(struct cx_stream* p_stream, int8_t* p_out);
int cx_stream_deserialize_int16(struct cx_stream* p_stream, int16_t* p_out);
int cx_stream_deserialize_int32(struct cx_stream* p_stream, int32_t* p_out);
int cx_stream_deserialize_int64(struct cx_stream* p_stream, int64_t* p_out);
int cx_stream_deserialize_float32(struct cx_stream* p_stream, float* p_out);
int cx_stream_deserialize_float64(struct cx_stream* p_stream, double* p_out);
int cx_stream_deserialize_string(struct cx_stream* p_stream, char* p_out_str, size_t* p_out_len);
int cx_stream_deserialize_cstring(struct cx_stream* p_stream, char* p_out_cstr, size_t* p_out_len);

#endif
