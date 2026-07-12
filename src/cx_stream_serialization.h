#ifndef CX_STREAM_SERIALIZATION_H
#define CX_STREAM_SERIALIZATION_H

#include <stddef.h>
#include <stdint.h>

struct cx_stream_writer;
struct cx_stream_reader;

int cx_stream_serialize_bytes(struct cx_stream_writer* p_writer, size_t size, const void* p_bytes);
int cx_stream_serialize_uint8(struct cx_stream_writer* p_writer, uint8_t value);
int cx_stream_serialize_uint16(struct cx_stream_writer* p_writer, uint16_t value);
int cx_stream_serialize_uint32(struct cx_stream_writer* p_writer, uint32_t value);
int cx_stream_serialize_uint64(struct cx_stream_writer* p_writer, uint64_t value);
int cx_stream_serialize_int8(struct cx_stream_writer* p_writer, int8_t value);
int cx_stream_serialize_int16(struct cx_stream_writer* p_writer, int16_t value);
int cx_stream_serialize_int32(struct cx_stream_writer* p_writer, int32_t value);
int cx_stream_serialize_int64(struct cx_stream_writer* p_writer, int64_t value);
int cx_stream_serialize_float32(struct cx_stream_writer* p_writer, float value);
int cx_stream_serialize_float64(struct cx_stream_writer* p_writer, double value);
int cx_stream_serialize_string(struct cx_stream_writer* p_writer, const char* p_str, size_t len);

int cx_stream_deserialize_bytes(struct cx_stream_reader* p_reader, size_t size, void* p_bytes);
int cx_stream_deserialize_uint8(struct cx_stream_reader* p_reader, uint8_t* p_out);
int cx_stream_deserialize_uint16(struct cx_stream_reader* p_reader, uint16_t* p_out);
int cx_stream_deserialize_uint32(struct cx_stream_reader* p_reader, uint32_t* p_out);
int cx_stream_deserialize_uint64(struct cx_stream_reader* p_reader, uint64_t* p_out);
int cx_stream_deserialize_int8(struct cx_stream_reader* p_reader, int8_t* p_out);
int cx_stream_deserialize_int16(struct cx_stream_reader* p_reader, int16_t* p_out);
int cx_stream_deserialize_int32(struct cx_stream_reader* p_reader, int32_t* p_out);
int cx_stream_deserialize_int64(struct cx_stream_reader* p_reader, int64_t* p_out);
int cx_stream_deserialize_float32(struct cx_stream_reader* p_reader, float* p_out);
int cx_stream_deserialize_float64(struct cx_stream_reader* p_reader, double* p_out);
int cx_stream_deserialize_string(struct cx_stream_reader* p_reader, char* p_out_str, size_t* p_out_len);

#endif
