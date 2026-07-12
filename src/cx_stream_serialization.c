#include <string.h>

#include "cx_macro.h"
#include "cx_stream.h"
#include "cx_stream_serialization.h"

int cx_stream_serialize_bytes(struct cx_stream_writer* p_writer, size_t size, const void* p_bytes) {
	return cx_stream_write(p_writer, size, p_bytes);
}

int cx_stream_serialize_uint8(struct cx_stream_writer* p_writer, uint8_t value) {
	return cx_stream_serialize_bytes(p_writer, 1, &value);
}

int cx_stream_serialize_uint16(struct cx_stream_writer* p_writer, uint16_t value) {
	return cx_stream_serialize_bytes(p_writer, 2, &value);
}

int cx_stream_serialize_uint32(struct cx_stream_writer* p_writer, uint32_t value) {
	return cx_stream_serialize_bytes(p_writer, 4, &value);
}

int cx_stream_serialize_uint64(struct cx_stream_writer* p_writer, uint64_t value) {
	return cx_stream_serialize_bytes(p_writer, 8, &value);
}

int cx_stream_serialize_int8(struct cx_stream_writer* p_writer, int8_t value) {
	return cx_stream_serialize_bytes(p_writer, 1, &value);
}

int cx_stream_serialize_int16(struct cx_stream_writer* p_writer, int16_t value) {
	return cx_stream_serialize_bytes(p_writer, 2, &value);
}

int cx_stream_serialize_int32(struct cx_stream_writer* p_writer, int32_t value) {
	return cx_stream_serialize_bytes(p_writer, 4, &value);
}

int cx_stream_serialize_int64(struct cx_stream_writer* p_writer, int64_t value) {
	return cx_stream_serialize_bytes(p_writer, 8, &value);
}

int cx_stream_serialize_float32(struct cx_stream_writer* p_writer, float value) {
	return cx_stream_serialize_bytes(p_writer, 4, &value);
}

int cx_stream_serialize_float64(struct cx_stream_writer* p_writer, double value) {
	return cx_stream_serialize_bytes(p_writer, 8, &value);
}

int cx_stream_serialize_string(struct cx_stream_writer* p_writer, const char* p_str, size_t len) {
	if (len == 0) {
		len = strlen(p_str);
	}

	return
		cx_stream_serialize_uint64(p_writer, len) &&
		cx_stream_serialize_bytes(p_writer, len, p_str);
}

int cx_stream_deserialize_bytes(struct cx_stream_reader* p_reader, size_t size, void* p_bytes) {
	return cx_stream_read(p_reader, size, p_bytes);
}

int cx_stream_deserialize_uint8(struct cx_stream_reader* p_reader, uint8_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 1, p_out);
}

int cx_stream_deserialize_uint16(struct cx_stream_reader* p_reader, uint16_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 2, p_out);
}

int cx_stream_deserialize_uint32(struct cx_stream_reader* p_reader, uint32_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 4, p_out);
}

int cx_stream_deserialize_uint64(struct cx_stream_reader* p_reader, uint64_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 8, p_out);
}

int cx_stream_deserialize_int8(struct cx_stream_reader* p_reader, int8_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 1, p_out);
}

int cx_stream_deserialize_int16(struct cx_stream_reader* p_reader, int16_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 2, p_out);
}

int cx_stream_deserialize_int32(struct cx_stream_reader* p_reader, int32_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 4, p_out);
}

int cx_stream_deserialize_int64(struct cx_stream_reader* p_reader, int64_t* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 8, p_out);
}

int cx_stream_deserialize_float32(struct cx_stream_reader* p_reader, float* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 4, p_out);
}

int cx_stream_deserialize_float64(struct cx_stream_reader* p_reader, double* p_out) {
	return cx_stream_deserialize_bytes(p_reader, 8, p_out);
}

int cx_stream_deserialize_string(struct cx_stream_reader* p_reader, char* p_out_str, size_t* p_out_len) {
	uint64_t temp;
	if (cx_stream_deserialize_uint64(p_reader, &temp)) {
		*p_out_len = temp;
		return cx_stream_deserialize_bytes(p_reader, *p_out_len, p_out_str);
	}
	return CX_FALSE;
}
