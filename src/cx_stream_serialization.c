#include <string.h>

#include "cx_macro.h"
#include "cx_stream.h"
#include "cx_stream_serialization.h"

int cx_stream_serialize_bytes(struct cx_stream* p_stream, size_t size, const void* p_bytes) {
	return cx_stream_write(p_stream, size, p_bytes);
}

int cx_stream_serialize_uint8(struct cx_stream* p_stream, uint8_t value) {
	return cx_stream_serialize_bytes(p_stream, 1, &value);
}

int cx_stream_serialize_uint16(struct cx_stream* p_stream, uint16_t value) {
	return cx_stream_serialize_bytes(p_stream, 2, &value);
}

int cx_stream_serialize_uint32(struct cx_stream* p_stream, uint32_t value) {
	return cx_stream_serialize_bytes(p_stream, 4, &value);
}

int cx_stream_serialize_uint64(struct cx_stream* p_stream, uint64_t value) {
	return cx_stream_serialize_bytes(p_stream, 8, &value);
}

int cx_stream_serialize_int8(struct cx_stream* p_stream, int8_t value) {
	return cx_stream_serialize_bytes(p_stream, 1, &value);
}

int cx_stream_serialize_int16(struct cx_stream* p_stream, int16_t value) {
	return cx_stream_serialize_bytes(p_stream, 2, &value);
}

int cx_stream_serialize_int32(struct cx_stream* p_stream, int32_t value) {
	return cx_stream_serialize_bytes(p_stream, 4, &value);
}

int cx_stream_serialize_int64(struct cx_stream* p_stream, int64_t value) {
	return cx_stream_serialize_bytes(p_stream, 8, &value);
}

int cx_stream_serialize_float32(struct cx_stream* p_stream, float value) {
	return cx_stream_serialize_bytes(p_stream, 4, &value);
}

int cx_stream_serialize_float64(struct cx_stream* p_stream, double value) {
	return cx_stream_serialize_bytes(p_stream, 8, &value);
}

int cx_stream_serialize_string(struct cx_stream* p_stream, const char* p_str, size_t len) {
	if (len == 0) {
		len = strlen(p_str);
	}

	return
		cx_stream_serialize_uint64(p_stream, len) &&
		cx_stream_serialize_bytes(p_stream, len, p_str);
}

int cx_stream_deserialize_bytes(struct cx_stream* p_stream, size_t size, void* p_bytes) {
	return cx_stream_read(p_stream, size, p_bytes);
}

int cx_stream_deserialize_uint8(struct cx_stream* p_stream, uint8_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 1, p_out);
}

int cx_stream_deserialize_uint16(struct cx_stream* p_stream, uint16_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 2, p_out);
}

int cx_stream_deserialize_uint32(struct cx_stream* p_stream, uint32_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 4, p_out);
}

int cx_stream_deserialize_uint64(struct cx_stream* p_stream, uint64_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 8, p_out);
}

int cx_stream_deserialize_int8(struct cx_stream* p_stream, int8_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 1, p_out);
}

int cx_stream_deserialize_int16(struct cx_stream* p_stream, int16_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 2, p_out);
}

int cx_stream_deserialize_int32(struct cx_stream* p_stream, int32_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 4, p_out);
}

int cx_stream_deserialize_int64(struct cx_stream* p_stream, int64_t* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 8, p_out);
}

int cx_stream_deserialize_float32(struct cx_stream* p_stream, float* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 4, p_out);
}

int cx_stream_deserialize_float64(struct cx_stream* p_stream, double* p_out) {
	return cx_stream_deserialize_bytes(p_stream, 8, p_out);
}

int cx_stream_deserialize_string(struct cx_stream* p_stream, char* p_out_str, size_t* p_out_len) {
	uint64_t temp;
	if (cx_stream_deserialize_uint64(p_stream, &temp)) {
		*p_out_len = temp;
		return cx_stream_deserialize_bytes(p_stream, *p_out_len, p_out_str);
	}
	return CX_FALSE;
}

int cx_stream_deserialize_cstring(struct cx_stream* p_stream, char* p_out_cstr, size_t* p_out_len) {
	const int b_result = cx_stream_deserialize_string(p_stream, p_out_cstr, p_out_len);
	if (b_result) {
		p_out_cstr[*p_out_len] = '\0';
	}
	return b_result;
}
