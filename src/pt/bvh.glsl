#pragma once

#ifndef MYRT_POINT_TYPE
#define MYRT_POINT_TYPE vec3
#endif

#ifndef MYRT_INDEX_TYPE
#define MYRT_INDEX_TYPE uint
#endif

#ifndef MYRT_BVH_NODE_PADDING_BYTES
#define MYRT_BVH_NODE_PADDING_BYTES 3
#endif

#ifndef MYRT_BVH_NODE_TYPE_SHIFT
#define MYRT_BVH_NODE_TYPE_SHIFT 31
#endif

struct MYRT_BVH_NODE_STRUCT {
    MYRT_POINT_TYPE min_extents;
    MYRT_INDEX_TYPE type_and_parent;
    MYRT_POINT_TYPE max_extents;
    MYRT_INDEX_TYPE first_child;
    MYRT_INDEX_TYPE second_child;
    uint m_padding[MYRT_BVH_NODE_PADDING_BYTES];
};
const uint type_shift = MYRT_BVH_NODE_TYPE_SHIFT;
const uint type_mask = 1 << type_shift;
const uint parent_mask = ~type_mask;

bool bvh_ray_aabb_intersect(MYRT_POINT_TYPE mi, MYRT_POINT_TYPE ma, vec3 o, vec3 id, float mt, inout float t)
{
    vec3 t135 = (mi - o) * id;
    vec3 t246 = (ma - o) * id;
    vec3 minv = min(t135, t246);
    vec3 maxv = max(t135, t246);
    float tmin = max(max(minv.x, minv.y), minv.z);
    float tmax = min(min(maxv.x, maxv.y), maxv.z);
    if (tmax >= 0 && tmin <= tmax && tmin <= mt) {
        t = tmin;
        return true;
    }
    return false;
}

bool bvh_node_is_leaf(const in MYRT_BVH_NODE_STRUCT node)
{
    return (node.type_and_parent & type_mask) != 0;
}

MYRT_INDEX_TYPE bvh_node_parent(const in MYRT_BVH_NODE_STRUCT node)
{
    return node.type_and_parent & parent_mask;
}
