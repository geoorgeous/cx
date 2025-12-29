#ifndef _H__MATERIAL
#define _H__MATERIAL

#include "asset.h"

#define ASSET_TYPE_MATERIAL 3

struct material {
    asset_handle p_texture;
    float        color[3];
};

#endif