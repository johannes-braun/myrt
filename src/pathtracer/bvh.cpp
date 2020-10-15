#include "bvh.hpp"
#include <numeric>
#include <algorithm>

namespace myrt
{
    struct build_state_t
    {
        aabb_t full_aabb;
        std::vector<aabb_t> aabbs;
    };

    namespace {
        std::pair<int, float> compute_split_axis(bvh_node_t const& node, build_state_t const& state)
        {
            aabb_t sub_aabb;
            auto const count = node.second_child - node.first_child + 1;
            for (auto index = node.first_child; index <= node.second_child; ++index)
            {
                const auto centroid = state.aabbs[index].centroid();
                sub_aabb.enclose(centroid);
            }

            auto const sub_aabb_size = sub_aabb.dimension();
            auto const step = sub_aabb_size / float(bvh::binned_sah_bin_count);

            std::array<std::array<aabb_t, bvh::binned_sah_bin_count>, 3> axis_bins{};
            for (auto index = node.first_child; index <= node.second_child; ++index)
            {
                const auto centroid = state.aabbs[index].centroid();
                glm::ivec3 const bin_id = glm::clamp(glm::ivec3((centroid - sub_aabb.min) / step), 
                    glm::ivec3(0), glm::ivec3(bvh::binned_sah_bin_count - 1));
                axis_bins[0][bin_id.x].enclose(state.aabbs[index]);
                axis_bins[1][bin_id.y].enclose(state.aabbs[index]);
                axis_bins[2][bin_id.z].enclose(state.aabbs[index]);
            }

            std::array<std::array<aabb_t, bvh::binned_sah_bin_count>, 3> aabbs_first{};
            std::array<std::array<aabb_t, bvh::binned_sah_bin_count>, 3> aabbs_second{};
            const auto enclose_reduce = [](aabb_t current, const aabb_t& next) -> aabb_t { current.enclose(next); return current; };
            for (int axis = 0; axis < 3; ++axis)
            {
                std::partial_sum(begin(axis_bins[axis]), end(axis_bins[axis]), begin(aabbs_first[axis]), enclose_reduce);
                std::partial_sum(rbegin(axis_bins[axis]), rend(axis_bins[axis]), rbegin(aabbs_second[axis]), enclose_reduce);
            }

            float best_surface_area_limit = std::numeric_limits<float>::max();
            int best_axis = 0;
            int best_axis_partition = 0;
            for (int axis = 0; axis < 3; ++axis)
            {
                for (size_t index = 0; index < bvh::binned_sah_bin_count; ++index)
                {
                    const auto first = aabbs_first[axis][index].surface_area();
                    const auto second = aabbs_second[axis][index].surface_area();
                    const auto max_surface = std::max(first, second);

                    if (max_surface < best_surface_area_limit)
                    {
                        best_axis = axis;
                        best_axis_partition = static_cast<int>(index);
                        best_surface_area_limit = max_surface;
                    }
                }
            }

            std::pair<int, float> result = std::make_pair(best_axis, (sub_aabb.min + step * float(best_axis_partition))[best_axis]);
            return result;
        }
    }

    [[nodiscard]] std::vector<aabb_t> generate_triangle_bounds(std::span<detail::default_index_type const> indices, std::function<detail::default_point_type(detail::default_index_type)> const& get_point)
    {
        std::vector<aabb_t> aabbs(indices.size() / 3);
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            const auto& p0 = get_point(indices[i + 0]);
            const auto& p1 = get_point(indices[i + 1]);
            const auto& p2 = get_point(indices[i + 2]);

            aabb_t& aabb = aabbs[i / 3];
            aabb.enclose(p0);
            aabb.enclose(p1);
            aabb.enclose(p2);
            aabb.weighted_centroid = (p0 + p1 + p2) / 3.0f;
        }
        return aabbs;
    }
    
    std::vector<aabb_t> myrt::generate_triangle_bounds(std::span<detail::default_index_type const> indices, std::span<detail::default_point_type const> points)
    {
        std::vector<aabb_t> aabbs(indices.size() / 3);
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            const auto& p0 = points[indices[i + 0]];
            const auto& p1 = points[indices[i + 1]];
            const auto& p2 = points[indices[i + 2]];

            aabb_t& aabb = aabbs[i / 3];
            aabb.enclose(p0);
            aabb.enclose(p1);
            aabb.enclose(p2);
            aabb.weighted_centroid = (p0 + p1 + p2) / 3.0f;
        }
        return aabbs;
    }

    bvh::bvh(std::span<aabb_t const> aabbs)
    {
        build_state_t build_state;
        build_state.aabbs = { aabbs.begin(),aabbs.end() };

        for (size_t i = 0; i < aabbs.size(); ++i)
            build_state.full_aabb.enclose(aabbs[i]);

        create(build_state);
    }

    std::experimental::generator<bvh::index_type> bvh::traverse(ray_t const& ray) const
    {
        using bit_stack = std::uint32_t;

        bit_stack visited_stack = 0;   // 1 -> node not fully done yet.
        bit_stack left_right_stack = 0;// 1 -> visited second,must visit first.

        index_type node_index = 0;

        const auto t_scene = ray.intersect(m_nodes[node_index].node.aabb());

        while (t_scene.has_value())
        {
            bvh_node_t const* current = &m_nodes[node_index].node;
            while (!current->is_leaf()) {
                bvh_node_t const& first = m_nodes[current->first_child].node;
                bvh_node_t const& second = m_nodes[current->second_child].node;
                const auto aabb_curr = current->aabb();
                const auto aabb_first = first.aabb();
                const auto aabb_right = second.aabb();

                const auto t_left = ray.intersect(first.aabb());
                const auto t_right = ray.intersect(second.aabb());

                if (!t_left.has_value() && !t_right.has_value())
                    break;

                left_right_stack = (t_left.has_value() && t_right.has_value() && t_left.value() > t_right.value()) | (left_right_stack << 1);
                visited_stack = (t_left.has_value() && t_right.has_value()) | (visited_stack << 1);

                node_index = (!t_left.has_value() || (t_right.has_value() && (left_right_stack & 0x1))) ? current->second_child : current->first_child;
                current = &m_nodes[node_index].node;
            }

            if (current->is_leaf())
                for (auto candidate = current->first_child; candidate <= current->second_child; ++candidate)
                    co_yield m_reordered_indices[candidate];

            while (!(visited_stack & 1))
            {
                if (!visited_stack)
                    co_return;

                node_index = m_nodes[node_index].node.parent();
                visited_stack >>= 1;
                left_right_stack >>= 1;
            }

            node_index = (left_right_stack & 0x1)
                ? m_nodes[m_nodes[node_index].node.parent()].node.first_child
                : m_nodes[m_nodes[node_index].node.parent()].node.second_child;
            visited_stack ^= 1;
        }
    }

    std::vector<aligned_node_t> const& bvh::nodes() const noexcept
    {
        return m_nodes;
    }

    std::vector<bvh::index_type> const& bvh::reordered_indices() const noexcept
    {
        return m_reordered_indices;
    }

    void bvh::create(build_state_t& initial_state)
    {
        m_reordered_indices.resize(initial_state.aabbs.size());
        std::iota(begin(m_reordered_indices), end(m_reordered_indices), index_type(0));
        m_nodes.reserve(2 * m_reordered_indices.size());

        index_type current_node_index = 0;
        ptrdiff_t active_nodes = 1;

        m_nodes.emplace_back().node = bvh_node_t{
            .min_extents = initial_state.full_aabb.min,
            .type_and_parent = bvh_node_t::make_type_and_parent(true,0),
            .max_extents = initial_state.full_aabb.max,
            .first_child = 0,
            .second_child = static_cast<index_type>(m_reordered_indices.size()) - 1
        };

        while (active_nodes--)
        {
            bvh_node_t& current_node = m_nodes[current_node_index].node;
            const auto split_nodes = split(current_node, initial_state);

            if (split_nodes.has_value())
            {
                const auto& [first, second] = split_nodes.value();
                active_nodes += 2;
                current_node.first_child = static_cast<index_type>(m_nodes.size());
                current_node.second_child = static_cast<index_type>(m_nodes.size() + 1);
                current_node.make_leaf(false);

                m_nodes.emplace_back(aligned_node_t{ .node = first }).node.set_parent(current_node_index);
                m_nodes.emplace_back(aligned_node_t{ .node = second }).node.set_parent(current_node_index);
            }
            current_node_index++;
        }
        m_nodes.shrink_to_fit();
    }
    std::optional<std::pair<bvh_node_t, bvh_node_t>> bvh::split(const bvh_node_t& current_node, build_state_t& build_state)
    {
        if (current_node.second_child - current_node.first_child < max_leaf_primitives)
            return std::nullopt;

        const auto [split_axis, split_plane] = compute_split_axis(current_node, build_state);

        const auto split_test = [&build_state, axis = split_axis, plane = split_plane](size_t index) {
            return build_state.aabbs[index].centroid()[axis] < plane;
        };

        // Partition
        auto first = current_node.first_child;
        auto last = current_node.second_child + 1;
        while (first != last && split_test(first)) first++;
        if (first != last)
        {
            for (auto i = first + 1; i != last; ++i) {
                if (split_test(i)) {
                    std::swap(m_reordered_indices[i], m_reordered_indices[first]);
                    std::swap(build_state.aabbs[i], build_state.aabbs[first]);
                    ++first;
                }
            }
        }
        const index_type split_index = first == current_node.first_child ? first + 1 : first;

        std::pair<bvh_node_t, bvh_node_t> split_result;
        split_result.first.first_child = current_node.first_child;
        split_result.first.second_child = split_index - 1;
        split_result.second.first_child = split_index;
        split_result.second.second_child = current_node.second_child;

        aabb_t first_aabb;
        for (auto i = split_result.first.first_child; i <= split_result.first.second_child; ++i)
            first_aabb.enclose(build_state.aabbs[i]);
        split_result.first.set_aabb(first_aabb);
        aabb_t second_aabb;
        for (auto i = split_result.second.first_child; i <= split_result.second.second_child; ++i)
            second_aabb.enclose(build_state.aabbs[i]);
        split_result.second.set_aabb(second_aabb);

        return split_result;
    }
    std::optional<float> ray_t::intersect(aabb_t const& aabb) const noexcept
    {
        glm::vec3 inv_direction = 1.f / direction;
        glm::vec3 t135 = (aabb.min - origin) * inv_direction;
        glm::vec3 t246 = (aabb.max - origin) * inv_direction;

        glm::vec3 min_values = min(t135, t246);
        glm::vec3 max_values = max(t135, t246);

        float tmin = glm::max(glm::max(min_values.x, min_values.y), min_values.z);
        float tmax = glm::min(glm::min(max_values.x, max_values.y), max_values.z);

        return (tmax >= 0 && tmin <= tmax && tmin <= length) ? std::optional(tmin < 0 ? tmax : tmin) : std::nullopt;
    }
    
    aabb_t bvh::aabb() const noexcept
    {
        return m_nodes[0].node.aabb();
    }

    namespace detail::glsl
    {
        constexpr size_t aligned_node_padding = sizeof(aligned_node_t) - sizeof(bvh_node_t);
        constexpr char aligned_node_padding_char = static_cast<char>((aligned_node_padding / sizeof(myrt::detail::default_index_type)) + '0');

        template<size_t S>
        class concat_string
        {
        public:
            template<size_t Next>
            [[nodiscard]] constexpr auto operator<<(char const (&text)[Next]) const noexcept
            {
                concat_string<S + Next - 1> result{};
                for (size_t i = 0; i < S - 1; ++i)
                    result.buffer[i] = buffer[i];
                for (size_t i = 0; i < Next; ++i)
                    result.buffer[i + S - 1] = text[i];
                return result;
            }
            template<size_t Next>
            [[nodiscard]] constexpr auto operator<<(concat_string<Next> const text) const noexcept
            {
                return operator<<(text.buffer);
            }
            [[nodiscard]] constexpr auto operator<<(char character) const noexcept
            {
                concat_string<S + 1> result{};
                for (size_t i = 0; i < S - 1; ++i)
                    result.buffer[i] = buffer[i];
                result.buffer[S - 1] = character;
                return result;
            }

            [[nodiscard]] constexpr std::string_view view() const noexcept {
                return std::string_view(buffer, S-1);
            }

            char buffer[S]{};
        };

        constexpr concat_string<1> make_str{};

        template<size_t TypeLength, size_t NameLength>
        constexpr auto var(char const (&type)[TypeLength], char const(&name)[NameLength])
        {
            return make_str << type << ' ' << name << ';';
        }

        constexpr auto bvh_nodes_struct = make_str
            << "\n#ifndef MYRT_BVH_BASIC_TYPES_DEFINED\n#define MYRT_BVH_BASIC_TYPES_DEFINED\nstruct " << bvh_struct_name << '{'
            << var(bvh_point_type, "min_extents")
            << var(bvh_index_type, "type_and_parent")
            << var(bvh_point_type, "max_extents")
            << var(bvh_index_type, "first_child")
            << var(bvh_index_type, "second_child")
            << "uint m_padding[" << aligned_node_padding_char << "];"
            << "};"
            << "const uint type_shift=" << ((bvh_node_t::type_shift / 10) % 10 ? (bvh_node_t::type_shift / 10) % 10 + '0' : ' ')
            << (bvh_node_t::type_shift % 10 ? bvh_node_t::type_shift % 10 + '0' : ' ') << ';'
            << "const uint type_mask=1<<type_shift;"
            << "const uint parent_mask=~type_mask;\n#endif\n"
            << "bool BVH_VISIT_PRIMITIVE(vec3 origin,vec3 direction," << bvh_index_type << " index,inout float max_ray_distance,out bool hits);"
            << "bool BVH_TRAVERSE(" << bvh_index_type << " node_base_index," << bvh_index_type << " primitive_base_index, vec3 ro,vec3 rd,float maxt);";

        constexpr auto bvh_traversal = make_str
            << "\n#ifndef BVH_HELPERS_DEFINED\n#define BVH_HELPERS_DEFINED\n" << "bool bvh_ray_aabb_intersect(" << bvh_point_type << " mi," << bvh_point_type << " ma,vec3 o,vec3 id,float mt,inout float t)"
            "{vec3 t135=(mi-o)*id;vec3 t246=(ma-o)*id;vec3 minv=min(t135,t246);vec3 maxv=max(t135,t246);"
            "float tmin=max(max(minv.x,minv.y),minv.z);float tmax=min(min(maxv.x,maxv.y),maxv.z);t=tmin<0?tmax:tmin;"
            "return tmax>=0&&tmin<=tmax&&tmin<=mt;}bool bvh_node_is_leaf(const in " << bvh_struct_name << " node){return (node.type_and_parent&type_mask)!=0;}"
            << bvh_index_type << " bvh_node_parent(const in " << bvh_struct_name << " node){return node.type_and_parent&parent_mask;}\n#endif\n"
            << "bool BVH_TRAVERSE("<<bvh_index_type<<" node_base_index,"<<bvh_index_type<<" primitive_base_index, vec3 ro,vec3 rd,float maxt)"
            "{bool hits_primitive=false;vec3 inv_rd=1.0/rd;uint visited_stack=0;uint left_right_stack=0;" << bvh_index_type << " node_index=node_base_index;float tmp=1.0/0.0;"
            "bool hits_any=bvh_ray_aabb_intersect(" << bvh_node_buffer_name << "[node_base_index].min_extents," << bvh_node_buffer_name << "[node_base_index].max_extents,ro,inv_rd,maxt,tmp);"
            "while(hits_any){" << bvh_struct_name << " current=" << bvh_node_buffer_name << "[node_index];while(!bvh_node_is_leaf(current)){float t_left,t_right;bool hits_left=bvh_ray_aabb_intersect("
            << bvh_node_buffer_name << "[current.first_child+node_base_index].min_extents," << bvh_node_buffer_name
            << "[current.first_child+node_base_index].max_extents,ro,inv_rd,maxt,t_left);bool hits_right=bvh_ray_aabb_intersect(" << bvh_node_buffer_name << "[current.second_child+node_base_index].min_extents,"
            << bvh_node_buffer_name << "[current.second_child+node_base_index].max_extents,ro,inv_rd,maxt,t_right);if(!hits_left&&!hits_right)"
            "break;left_right_stack=uint(t_left>t_right)|(left_right_stack<<1);visited_stack=uint(hits_left&&hits_right)|(visited_stack<<1);"
            "bool use_right=(!hits_left||(hits_right&&((left_right_stack&0x1)==0x1)));node_index=(use_right?current.second_child:current.first_child)+node_base_index;current="
            << bvh_node_buffer_name << "[node_index];}if(bvh_node_is_leaf(current))"
            "{for("<<bvh_index_type<<" tri=current.first_child;tri<=current.second_child;++tri){bool h=false;if(BVH_VISIT_PRIMITIVE(ro,rd," << bvh_indices_buffer_name << "[primitive_base_index+tri],maxt,h))return true;if(h)hits_primitive=true;}}"
            "while((visited_stack&1)!=1){if(visited_stack==0)"
            "return hits_primitive;node_index=bvh_node_parent(" << bvh_node_buffer_name << "[node_index])+node_base_index;visited_stack>>=1;left_right_stack>>=1;}"
            "node_index=node_base_index + (((left_right_stack&0x1)==0x1)?" << bvh_node_buffer_name << "[bvh_node_parent(" << bvh_node_buffer_name << "[node_index])+node_base_index].first_child:"
            << bvh_node_buffer_name << "[bvh_node_parent(" << bvh_node_buffer_name << "[node_index])+node_base_index].second_child);visited_stack^=1;}}";

        constexpr auto full_bvh_code = make_str
            << bvh_traversal;

        constexpr auto intersect_triangle = make_str
            << "bool intersect_triangle(const vec3 origin,const vec3 direction,const vec3 v1,const vec3 v2,const vec3 v3,inout float t,inout vec2 barycentric)"
            "{float float_epsilon=1e-23f;float border_epsilon=1e-6f;vec3 e1=v2-v1;vec3 e2=v3-v1;"
            "vec3 P=cross(vec3(direction),e2);float det=dot(e1,P);if(det>-float_epsilon && det<float_epsilon)return false;"
            "float inv_det=1.0/det;vec3 T=origin.xyz-v1;barycentric.x=dot(T,P)*inv_det;"
            "if(barycentric.x<-border_epsilon||barycentric.x>1.0+border_epsilon)return false;"
            "vec3 Q=cross(T,e1);barycentric.y=dot(vec3(direction),Q)*inv_det;"
            "if(barycentric.y<-border_epsilon||barycentric.x+barycentric.y>1.0+border_epsilon)return false;"
            "float tt=dot(e2,Q)*inv_det;return(t=tt)>float_epsilon;}";
    }

    namespace glsl
    {
        namespace {
            [[nodiscard]] std::string def(std::string const& name, std::string const& value)
            {
                return "\n#define " + name + " " + value + "\n";
            }
            [[nodiscard]] std::string undef(std::string const& name)
            {
                return "\n#undef " + name + "\n";
            }
        }

        [[nodiscard]] std::string bvh_definitions_code(std::string const& traversal_name, std::string const& primitive_name)
        {
            return def("BVH_VISIT_PRIMITIVE", primitive_name) + 
                def("BVH_TRAVERSE", traversal_name) + 
                std::string(detail::glsl::bvh_nodes_struct.view())+ 
                undef("BVH_TRAVERSE") +
                undef("BVH_VISIT_PRIMITIVE");
        }
        [[nodiscard]] std::string bvh_code(std::string const& traversal_name, std::string const& primitive_name, std::string const& nodes, std::string const& indices)
        {
            return def(detail::glsl::bvh_node_buffer_name, nodes) + 
                def(detail::glsl::bvh_indices_buffer_name, indices) + 
                def("BVH_VISIT_PRIMITIVE", primitive_name) + 
                def("BVH_TRAVERSE", traversal_name) +
                std::string(detail::glsl::full_bvh_code.view()) + 
                undef("BVH_TRAVERSE") + 
                undef("BVH_VISIT_PRIMITIVE") + 
                undef(detail::glsl::bvh_indices_buffer_name) + 
                undef(detail::glsl::bvh_node_buffer_name);
        }
        [[nodiscard]] std::string intersect_triangle_code()
        {
            return std::string(detail::glsl::intersect_triangle.view());
        }
    }
}