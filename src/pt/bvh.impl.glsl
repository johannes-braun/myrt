#ifndef UINT_TYPE
#define UINT_TYPE uint
#endif

bool MYRT_BVH_VISIT_PRIMITIVE(vec3 origin, vec3 direction,
    MYRT_INDEX_TYPE index,
    inout float max_ray_distance, out bool hits);
bool MYRT_BVH_TRAVERSE(MYRT_INDEX_TYPE node_base_index,
    MYRT_INDEX_TYPE primitive_base_index, vec3 ro, vec3 rd,
    float maxt)
{
    bool hits_primitive = false;
    vec3 inv_rd = 1.0 / rd;
    UINT_TYPE visited_stack = 0;
    UINT_TYPE left_right_stack = 0;
    MYRT_INDEX_TYPE node_index = node_base_index;
    float tmp = 1.0 / 0.0;
    bool hits_any = bvh_ray_aabb_intersect(MYRT_BVH_NODES_BUFFER[node_base_index].min_extents,
        MYRT_BVH_NODES_BUFFER[node_base_index].max_extents,
        ro, inv_rd, maxt, tmp);
    while (hits_any) {
        MYRT_BVH_NODE_STRUCT current = MYRT_BVH_NODES_BUFFER[node_index];
        while (!bvh_node_is_leaf(current)) {
            float t_left, t_right;

            MYRT_BVH_NODE_STRUCT first_child = MYRT_BVH_NODES_BUFFER[current.first_child + node_base_index];
            MYRT_BVH_NODE_STRUCT second_child = MYRT_BVH_NODES_BUFFER[current.second_child + node_base_index];

            bool hits_left = bvh_ray_aabb_intersect(
                first_child.min_extents,
                first_child.max_extents,
                ro, inv_rd, maxt, t_left);
            bool hits_right = bvh_ray_aabb_intersect(
                second_child.min_extents,
                second_child.max_extents,
                ro, inv_rd, maxt, t_right);
            if (!hits_left && !hits_right)
                break;
            bool use_right = !hits_left || (hits_right && t_left >= t_right);
            left_right_stack = UINT_TYPE(use_right) | (left_right_stack << UINT_TYPE(1));
            visited_stack = UINT_TYPE(hits_left && hits_right) | (visited_stack << UINT_TYPE(1));
            node_index = (use_right ? current.second_child : current.first_child) + node_base_index;
            current = MYRT_BVH_NODES_BUFFER[node_index];
        }
        if (bvh_node_is_leaf(current)) {
            for (MYRT_INDEX_TYPE tri = current.first_child;
                tri <= current.second_child; ++tri) {
                bool h = false;
                if (MYRT_BVH_VISIT_PRIMITIVE(
                    ro, rd, MYRT_BVH_INDICES_BUFFER[primitive_base_index + tri],
                    maxt, h))
                    return true;
                if (h)
                {
                    hits_primitive = true;
                }
            }
        }
        while ((visited_stack & UINT_TYPE(1)) != 1) {
            if (visited_stack == 0)
                return hits_primitive;
            node_index = bvh_node_parent(MYRT_BVH_NODES_BUFFER[node_index]) + node_base_index;
            visited_stack >>= UINT_TYPE(1);
            left_right_stack >>= UINT_TYPE(1);
        }
        node_index = node_base_index + (((left_right_stack & 0x1) == 0x1)
            ? MYRT_BVH_NODES_BUFFER[bvh_node_parent(MYRT_BVH_NODES_BUFFER[node_index]) + node_base_index].first_child
            : MYRT_BVH_NODES_BUFFER[bvh_node_parent(MYRT_BVH_NODES_BUFFER[node_index]) + node_base_index].second_child);
        visited_stack ^= UINT_TYPE(1);
    }
    return false;
}