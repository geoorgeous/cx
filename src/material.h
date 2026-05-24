#ifndef MATERIAL_H
#define MATERIAL_H

#include "cx_asset.h"

#define ASSET_TYPE_MATERIAL 3

struct material {
    cx_asset_handle p_texture;
    float        color[3];
};

#endif
