#include "cx_alloc.h"
#include "cx_image.h"
#include "cx_macro.h"
#include "cx_stream.h"
#include "cx_stream_serialization.h"

int cx_image_serialize(const struct cx_image* p_image, struct cx_stream* p_stream) {
	cx_stream_serialize_uint32(p_stream, p_image->width);
	cx_stream_serialize_uint32(p_stream, p_image->height);
	cx_stream_serialize_uint8(p_stream, (uint8_t)p_image->pixel_data_format.pixel_format);
	cx_stream_serialize_uint8(p_stream, (uint8_t)p_image->pixel_data_format.pixel_type);
	cx_stream_serialize_bytes(p_stream, cx_image_pixel_data_size(p_image), p_image->p_pixel_data);
	return CX_TRUE;
}

int cx_image_deserialize(struct cx_stream* p_stream, struct cx_image* p_out_image) {
	cx_stream_deserialize_uint32(p_stream, &p_out_image->width);
	cx_stream_deserialize_uint32(p_stream, &p_out_image->height);

	uint8_t temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_out_image->pixel_data_format.pixel_format = (enum cx_pixel_format)temp;

	cx_stream_deserialize_uint8(p_stream, &temp);
	p_out_image->pixel_data_format.pixel_type = (enum cx_pixel_type)temp;

	cx_stream_deserialize_bytes(p_stream, cx_image_pixel_data_size(p_out_image), p_out_image->p_pixel_data);

	return CX_TRUE;
}

void cx_image_asset_destroy(void* p_asset) {
	CX_FREE(((struct cx_image*)p_asset)->p_pixel_data);
}
