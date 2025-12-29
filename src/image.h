#ifndef _H__IMAGE
#define _H__IMAGE

#define ASSET_TYPE_IMAGE 1

struct image {
    unsigned int width;
    unsigned int height;
    unsigned int num_channels;
    void*        p_pixel_data;
};

#endif