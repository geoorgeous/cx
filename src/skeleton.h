#ifndef _H__SKELETON
#define _H__SKELETON

#include <stdint.h>

#define ASSET_TYPE_SKELETON 6

struct skeleton_joint {
    size_t* p_children_indices;
    size_t  num_children;
    size_t  parent_index;
    float   transform[16];
};

struct skeleton {
    struct skeleton_joint* p_joints;
    size_t                 num_joints;
    size_t*                p_root_joints_indices;
    size_t                 num_root_joints;
    float*                 p_inverse_bind_matrices;
};

struct skeleton_instance {
    struct skeleton* p_skeleton;
    float*           p_joint_matrices;
};

#endif