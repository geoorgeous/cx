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

struct cx_stream_writer;
struct cx_stream_reader;

int cx_image_serialize(const struct cx_image* p_image, struct cx_stream_writer* p_writer);
int cx_image_deserialize(struct cx_image* p_image, struct cx_stream_reader* p_reader);

void cx_image_asset_destroy(void* p);

static inline int cx_image_asset_serialize(struct cx_stream_writer* p_writer, const void* p) {
	return cx_image_serialize(p, p_writer);
}

static inline int cx_image_asset_deserialize(struct cx_stream_reader* p_reader, void* p) {
	return cx_image_deserialize(p, p_reader);
}

#endif
