#ifndef CX_IMAGE_H
#define CX_IMAGE_H

#define ASSET_TYPE_IMAGE 1

#include <stdint.h>

#include "cx_pixel_format.h"

struct cx_image {
	uint32_t                      width;
	uint32_t                      height;
	struct cx_pixel_buffer_format pixel_data_format;
	void*                         p_pixel_data;
};

static inline size_t cx_image_pixel_data_size(const struct cx_image* p_image) {
	return p_image->width * p_image->height * cx_pixel_buffer_format_compute_pixel_size(&p_image->pixel_data_format);
}

struct cx_stream;

int cx_image_serialize(const struct cx_image* p_image, struct cx_stream* p_stream);
int cx_image_deserialize(struct cx_stream* p_stream, struct cx_image* p_out_image);
void cx_image_asset_destroy(void* p_asset);

static inline int cx_image_asset_serialize(const void* p_asset, struct cx_stream* p_stream) {
	return cx_image_serialize(p_asset, p_stream);
}

static inline int cx_image_asset_deserialize(struct cx_stream* p_stream, void* p_out_asset) {
	return cx_image_deserialize(p_stream, p_out_asset);
}

#endif
