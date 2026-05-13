#ifndef IMAGE_H
#define IMAGE_H

#define ASSET_TYPE_IMAGE 1

#include <stdint.h>

#include "cx_pixel_format.h"

struct image {
	uint32_t                      width;
	uint32_t                      height;
	struct cx_pixel_buffer_format pixel_data_format;
    void*                         p_pixel_data;
};

#endif
