#ifndef _H__TEXTURE_SAMPLER
#define _H__TEXTURE_SAMPLER

enum texture_filter_min {
    TEXTURE_FILTER_MIN_nearest,
    TEXTURE_FILTER_MIN_linear,
    TEXTURE_FILTER_MIN_nearest_mipmap_nearest,
    TEXTURE_FILTER_MIN_linear_mipmap_nearest,
    TEXTURE_FILTER_MIN_nearest_mipmap_linear,
    TEXTURE_FILTER_MIN_linear_mipmap_linear
};

enum texture_filter_mag {
    TEXTURE_FILTER_MAG_nearest,
    TEXTURE_FILTER_MAG_linear
};

enum texture_address_mode {
    TEXTURE_ADDRESS_MODE_clamp_to_edge,
    TEXTURE_ADDRESS_MODE_mirrored_repeat,
    TEXTURE_ADDRESS_MODE_repeat
};

struct texture_sampler {
    enum texture_filter_min   filter_min;
    enum texture_filter_mag   filter_mag;
    enum texture_address_mode address_mode_u;
    enum texture_address_mode address_mode_v;
};

#endif